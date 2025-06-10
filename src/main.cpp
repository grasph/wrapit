//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <tuple>
#include <cxxopts.hpp>
#include <ctime>
#include <unistd.h>

#include "toml.hpp"
using namespace std::string_view_literals;

#include "CodeTree.h"
#include "utils.h"
#include "uuid_utils.h"
#include "cxxwrap_version.h"

using namespace codetree;

extern const char* version;

namespace fs = std::filesystem;

extern int verbose;

// print help options
void print_help(const cxxopts::Options& options)
{
  std::cout << options.help({"", "compilation"}) << '\n';
}

// print error message
void print_error(const std::string& msg)
{
  std::cerr << msg << '\n';
}

std::string join(const std::string& sep, const std::vector<std::string> to_join){
  std::stringstream buf;
  std::string empty("");
  const std::string* sep_ = &empty;
  for(const auto& s: to_join){
    buf << (*sep_) << s;
    sep_ = &sep;
  }
  return buf.str();
}

std::tuple<fs::path, bool>
find_include_path(const fs::path& p, const std::vector<fs::path>& inc_dirs){
  bool found = false;
  fs::path pp;
  if(p.is_absolute()){
    try{
      pp = fs::canonical(p);
      found = true;
    } catch(fs::filesystem_error){
      /* NO-OP */
    }
  } else{
    for(const auto& d: inc_dirs){
      try{
        pp =  fs::canonical(d / p);
        found = true;
      } catch(fs::filesystem_error){
        //path does not exist
        /* NO-OP */
      }
    }
  }
  return std::make_tuple(pp, found);
}


void append_to_table(toml::table& dest, toml::table& src){
  for (const auto& [key, value]: src) {
    dest.insert_or_assign(key, value);
  }
}

int main(int argc, char* argv[]){

  srand(time(nullptr) ^ ~getpid());

  cxxopts::Options option_list("wrapit",
                               "Generates wrappers from a c++ header file for Cxx.jl.\n");
  
  // clang-format off
  option_list.add_options()
    ("h,help", "Display this help and exit")
    ("v,verbosity", "Set verbosity leval",
     cxxopts::value<int>()->default_value("0"))
    ("force", "Force overwriting output files.")
    ("cfgfile", "Configuration file (in toml format)",
     cxxopts::value<std::string>())
    ("cmake", "Generate a wrapit.cmake file to be included in a CMake "
     "configuration file. It defines two variables WRAPIT_INPUTS and "
     "WRAPIT_PRODUCTS repectively with the list of input header files "
     "the result depends on and the list of produced files")
    ("get", "Retrieves a configuration parameter. Retrieval of only few "
     "parameters are currently supported.",
     cxxopts::value<std::string>())
    ("add-cfg", "Set a configuration parameter on top of what is defined in the"
     " configuration file.",
     cxxopts::value<std::vector<std::string>>())
    ("output-prefix", "Prefix inserted to output paths",
     cxxopts::value<std::string>()->default_value(""))
    ("resource-dir", std::string("Change the clang resource directory path (see clang "
                                 "--help and clang --print-resource-dir). Default: ")
     + CodeTree::resolve_clang_resource_dir_path(CLANG_RESOURCE_DIR) + ".",
     cxxopts::value<std::string>())
    ("u,update", "Enable update mode. In update mode, if a file to generate "
     "already exist and there is no code changed, then the file including "
     "its time stamp is preserved. The time stamp can then be used to "
     "recompile modified files only during wrapper development of "
     "large projects.")
    ("V,version", "Display the software version")
    ;

  option_list.parse_positional({"cfgfile"});

  auto options = option_list.parse(argc, argv);
  if (options.count("help")){
    print_help(option_list);
  } else if(options.count("version")){
    std::cout << "WrapIt! version " << version << "\n";
  } else {
    toml::parse_result toml_config;
    try{
      toml_config = toml::parse_file(options["cfgfile"].as<std::string>());
    } catch(const toml::v3::ex::parse_error& ex){
      std::cerr << "Failed to read the configuration file "
                <<  options["cfgfile"].as<std::string>()
                << "\n\t" << ex.what() <<  " at " << ex.source() << ".\n";
      return 1;
    }

    auto read_vstring = [toml_config](const char* datacard, std::vector<std::string> defVal = std::vector<std::string>()){
      auto config_ = toml_config.get_as<toml::array>(datacard);
      std::vector<std::string> values;
      if(config_){
        for(const auto& v: *config_) values.push_back(**(v.as<std::string>()));
      } else{
        values.resize(defVal.size());
        std::copy(defVal.begin(), defVal.end(), values.begin());
      }
      return values;
    };

    auto read_vpath = [toml_config](const char* datacard, std::vector<fs::path> defaults = std::vector<fs::path>()){
      auto config_ = toml_config.get_as<toml::array>(datacard);
      if(config_){
        std::vector<fs::path> values;
        for(const auto& v: *config_) values.push_back(**(v.as<std::string>()));
        return values;
      } else{
        return defaults;
      }
    };

    if(options["add-cfg"].count() > 0){
      auto params= options["add-cfg"].as<std::vector<std::string>>();
      for(const auto& p: params){
        toml::table to_append;
        try{
          to_append = toml::parse(p);
        } catch (const toml::parse_error& err) {
          std::cerr << "Error parsing --add-cfg parameter, '" << p << "': "
                    << err.what() << "\n";
          exit(1);
        }
        try{
          append_to_table(toml_config, to_append);
        } catch (const toml::parse_error& err) {
        std::cerr << "Failed to set parameter specified by --add-cfg, '"
                  << p << "': " << err.what() << "\n";
        exit(1);
        } 
      }
    }

    auto cxxwrap_version_str = toml_config["cxxwrap_version"].value_or(std::string(""));
    std::string mess;
    if(cxxwrap_version_str.size() == 0){//default version
      mess = "Error. Bug found in wrapit. Cxx wrap version string defined in the file cxxwrap_version.h is not valid.";
      cxxwrap_version_str = default_cxxwrap_version;
    } else{
      std::stringstream buf;
      buf << "Error. The format of the value of the cxxwrap_version "
        "configuration parameter (\"" << cxxwrap_version_str
          << "\") is not valid. Expected format: x.y.z, x.y, or x";
      mess = buf.str();
    }
    auto cxxwrap_version = version_string_to_int(cxxwrap_version_str);
    if(cxxwrap_version < 0){
      std::cerr << mess;
      exit(1);
    }
    
    auto include_dirs = read_vpath("include_dirs", { fs::path(".")} );
    //auto to_parse = read_vpath_include("input", include_dirs);
    auto to_parse = read_vstring("input");
    auto extra_headers = read_vstring("extra_headers");

    auto inheritances = read_vstring("inheritances");
    auto vetoed_finalizer_classes  = read_vstring("vetoed_finalizer_classes");
    auto vetoed_copy_ctor_classes  = read_vstring("vetoed_copy_ctor_classes");

    auto module_name = toml_config["module_name"].value_or(std::string("CxxLib"));
    auto out_export_jl_fname = toml_config["export_jl_fname"].value_or(std::string());
    auto out_jl_fname = toml_config["module_jl_fname"].value_or(std::string());
    if(out_jl_fname.size() == 0) out_jl_fname = module_name + ".jl";
    auto cxx_std = toml_config["cxx-std"].value_or(std::string("c++17"));

    auto out_project_fname = toml_config["project_toml_fname"].value_or(std::string("Project.toml"));

    auto macro_definitions = read_vstring("macro_definitions", std::vector<std::string>(1, std::string("WRAPIT")));
    auto clang_features = read_vstring("clang_features");
    auto clang_opts     = read_vstring("clang_opts");

    auto lib_basename       = toml_config["lib_basename"].value_or(std::string("$(@__DIR__)/../deps/libjl") + module_name);

    std::string output_prefix = options["output-prefix"].as<std::string>();

    auto resolve_out_dir = [&](const std::string & dir){
      if(output_prefix.size() == 0 || fs::path(dir).is_absolute()) return dir;
      else return (std::string)(fs::path(output_prefix) / fs::path(dir));
    };

    auto out_cxx_dir        = resolve_out_dir(toml_config["out_cxx_dir"].value_or(join_paths("lib" + module_name, std::string("src"))));
    auto out_jl_dir         = resolve_out_dir(toml_config["out_jl_dir"].value_or(module_name));
    auto out_jl_subdir      = toml_config["out_jl_subdir"].value_or("src");
    auto out_report_fpath   = resolve_out_dir(std::string("jl") + module_name + "-report.txt");
    std::string out_cmake_fpath;
    if(options.count("cmake") > 0){
      out_cmake_fpath = resolve_out_dir("wrapit.cmake");
    }

    auto n_classes_per_file = toml_config["n_classes_per_file"].value_or(-1);

    auto julia_names = read_vstring("julia_names");

    auto mapped_types = read_vstring("mapped_types");

    auto class_order_contraints = read_vstring("class_order_constraints");
    
    auto veto_list = toml_config["veto_list"].value_or(""sv);

    auto auto_veto = toml_config["auto_veto"].value_or(true);

    auto propagation_mode  = toml_config["propagation_mode"].value_or("types"sv);

    auto build_cmd  = toml_config["build_cmd"].value_or("echo No build command defined"sv);

    auto test_build = toml_config["test_build"].value_or(false);

    auto build_nskips = toml_config["build_nskips"].value_or(0);
    auto build_nmax = toml_config["build_nmax"].value_or(-1);
    auto build_every = toml_config["build_every"].value_or(1);

    auto fields_and_variables = toml_config["fields_and_variables"].value_or(true);

    auto verbosity = options["verbosity"].as<int>();
    //toml_config["verbosity"].value_or(0);

    if(propagation_mode != "types"
       && propagation_mode != "methods"){
      std::cerr << "Warning: value '" << propagation_mode
                << "' for configurable propagation_mode is not valid. "
        "Valid values: types, methods.\n";
      propagation_mode = "types";
    }


    auto export_mode      = toml_config["export"].value_or(std::string("member_functions"));
    auto export_blacklist = read_vstring("export_blacklist");

    if(export_mode != "none"
       && export_mode != "member_functions"
       && export_mode != "all_functions"
       && export_mode != "all"){
      std::cerr << "Warning: value '" << export_mode
                << "' for configurable export is not valid. "
        "Valid values: none, all_functions, all.\n";
      export_mode = "member_functions";
    }


    auto version = toml_config["version"].value_or(std::string());

    if(options.count("get")){
      auto param = options["get"].as<std::string>();
      if(param == "cxxwrap_version")      std::cout << cxxwrap_version_str << "\n";
      else if(param == "module_name")     std::cout << module_name << "\n";
      else if(param == "version")         std::cout << version << "\n";
      else if(param == "lib_basname")     std::cout << lib_basename << "\n";
      else if(param == "export_jl_fname") std::cout << out_export_jl_fname << "\n";
      else if(param == "module_jl_fname") std::cout << out_jl_fname << "\n";
      else std::cerr << "Retrieval of parameter '" << param
                     << "' is not supported";
      exit(0);
    }

    auto uuid = toml_config["uuid"].value_or(std::string());

    if(uuid.size() == 0){
      uuid = gen_uuid();
      std::cerr << "The configuration file misses the uuid parameter. Following "
        "generated uuid will be used for the generate Julia project. Add the "
        "following line in the .wit configuration file to use same uuid "
        "for next versions of the code.\n"
        "uuid = \"" << uuid << "\"\n\n";
    } else if (!validate_uuid(uuid)){
      std::cerr << "The value \"" << uuid << "\" of the uuid parameter found in "
        "the .wit configuration file is not valid. You can run first with an "
        "empty string to generate a new uuid, which will be displayed to the "
        "output (stderr).\n";
        return 1;
    }    

    bool in_err = false;
    auto open_mode = std::ofstream::out;
    if(options.count("force") == 0){
      open_mode |= std::ofstream::app;
    }
    
    auto open_file = [&](const std::string& fname){
      std::ofstream f(fname, open_mode);
      if(f.tellp()!=0){
        std::cerr << "File " << fname << " is in the way, please move it or use the --force option to force its deletion.\n";
        in_err = true;
      }
      return f;
    };


    auto out_jl_src = join_paths(out_jl_dir, out_jl_subdir);
    fs::create_directories(out_jl_src);
    auto out_jl = open_file(join_paths(out_jl_src, out_jl_fname));

    std::ofstream out_export_jl_;
    bool same_ = true;
    if(out_export_jl_fname.size() > 0){
      same_ = false;
      out_export_jl_ = std::move(open_file(join_paths(out_jl_src, out_export_jl_fname)));
    }
    auto& out_export_jl = same_ ? out_jl : out_export_jl_;

    std::ofstream out_report(out_report_fpath, open_mode);
    if(out_report.tellp()!=0){
      std::cerr << "File " << out_report_fpath
                << " is in the way, please move it or use the --force option to force its deletion.\n";
      in_err = true;
    }

    auto out_project_fpath = join_paths(out_jl_dir, out_project_fname);
    std::ofstream out_project_toml(out_project_fpath, open_mode);
    if(out_project_toml.tellp()!=0){
      std::cerr << "File " << out_project_fpath
                << " is in the way, please move it or use the --force option to force its deletion.\n";
      in_err = true;
    }

    if(in_err) return -1;



    verbose = verbosity;

    CodeTree tree;

    tree.set_force_mode(options.count("force") > 0);

    tree.set_cxxwrap_version(cxxwrap_version);

    tree.set_julia_names(julia_names);
    tree.set_mapped_types(mapped_types);
    tree.set_class_order_constraints(class_order_contraints);
    
    tree.add_std_option(cxx_std);

    tree.auto_veto(auto_veto);
    tree.cmake(out_cmake_fpath);
    tree.enableTestBuild(test_build);
    tree.build_nskips(build_nskips);
    tree.build_nmax(build_nmax);
    tree.build_every(build_every);
    tree.build_cmd(std::string(build_cmd));
    tree.inheritances(inheritances);
    tree.vetoed_finalizer_classes(vetoed_finalizer_classes);
    tree.vetoed_copy_ctor_classes(vetoed_copy_ctor_classes);
    tree.accessor_generation_enabled(fields_and_variables);

    tree.set_n_classes_per_file(n_classes_per_file);

    tree.set_module_name(module_name);

    tree.set_out_cxx_dir(out_cxx_dir);
    tree.set_out_jl_dir(out_jl_dir);

    if(propagation_mode == "types"){
      tree.propagation_mode(propagation_mode_t::types);
    } else if(propagation_mode == "methods"){
      tree.propagation_mode(propagation_mode_t::methods);
    }

    if(export_mode == "none"){
      tree.export_mode(export_mode_t::none);
    } else if(export_mode == "member_functions"){
      tree.export_mode(export_mode_t::member_functions);
    } else if(export_mode == "all_functions"){
      tree.export_mode(export_mode_t::all_functions);
    } else if(export_mode == "all"){
      tree.export_mode(export_mode_t::all);
    }

    for (const auto& include : include_dirs){
      tree.add_include_dir(include.string());
    }

    for (const auto& macro: macro_definitions){
      tree.define_macro(macro);
    }

    for (const auto& name: clang_features){
      if(name.size() > 0){
        tree.enable_feature(name);
      }
    }

    for (const auto& name: clang_opts){
      if(name.size() > 0){
        tree.add_clang_opt(name);
      }
    }

    if(options.count("resource-dir")){
      tree.set_clang_resource_dir(options["resource-dir"].as<std::string>());
    }

    if(options.count("update")){
      tree.set_update_mode(true);
    }

    for(const auto& s: extra_headers){
      tree.add_forced_header(s);
    }

    if(veto_list.size() > 0){
      tree.parse_vetoes(fs::path(veto_list));
    }

    for(const auto& p: to_parse){
      tree.add_source_file(p);
    }

    for(const auto& k: export_blacklist){
      tree.add_export_veto_word(k);
    }

    tree.parse();
    tree.preprocess();
    tree.generate_cxx();
    tree.generate_jl(out_jl, out_export_jl, module_name, lib_basename);
    tree.generate_project_file(out_project_toml, uuid, version);

    tree.report(out_report);
  }
}
