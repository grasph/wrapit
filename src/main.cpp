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

#include "toml.hpp"
using namespace std::string_view_literals;

#include "CodeTree.h"

using namespace codetree;

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



int main(int argc, char* argv[]){
  cxxopts::Options option_list("wrapit",
                               "Generates wrappers from a c++ header file for Cxx.jl.\n");
  // clang-format off
  option_list.add_options()
    ("h,help", "display this help and exit")
    ("v,verbosity", "set verbosity leval",
     cxxopts::value<int>()->default_value("0"))
    ("force", "force overwriting output files.")
    ("cfgfile", "configuration file (in toml format)",
     cxxopts::value<std::string>());

  option_list.parse_positional({"cfgfile"});

  auto options = option_list.parse(argc, argv);
  if (options.count("help")){
    print_help(option_list);
  } else {
    toml::parse_result toml_config;
    try{
      toml_config = toml::parse_file(options["cfgfile"].as<std::string>());
    } catch(const toml::v2::ex::parse_error& ex){
      std::cerr << "Failed to read the configuration file "
                <<  options["cfgfile"].as<std::string>()
                << "\n\t" << ex.what() <<  " at " << ex.source() << ".\n";
      return 1;
    }

    auto read_vstring = [toml_config](const char* datacard){
      auto config_ = toml_config.get_as<toml::array>(datacard);
      std::vector<std::string> values;
      if(config_){
        for(const auto& v: *config_) values.push_back(**(v.as<std::string>()));
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

    auto include_dirs = read_vpath("include_dirs", { fs::path(".")} );
    //auto to_parse = read_vpath_include("input", include_dirs);
    auto to_parse = read_vstring("input");
    auto extra_headers = read_vstring("extra_headers");

    auto inheritances = read_vstring("inheritances");
    auto vetoed_finalizer_classes  = read_vstring("vetoed_finalizer_classes");

    auto module_name = toml_config["module_name"].value_or(std::string("CxxLib"));
    auto out_export_jl_fname = toml_config["export_jl_fname"].value_or(std::string());
    auto out_jl_fname = toml_config["module_jl_fname"].value_or(std::string());
    if(out_jl_fname.size() == 0) out_jl_fname = module_name + ".jl";
    auto cxx_std = toml_config["cxx-std"].value_or(std::string("c++17"));

    auto macro_definitions = read_vstring("macro_definitions");
    auto clang_features = read_vstring("clang_features");
    auto clang_opts     = read_vstring("clang_opts");

    auto lib_basename     = std::string("jl") + module_name;
    auto out_cpp_fname    = toml_config["out_cpp_fname"].value_or(std::string("jl") + module_name + ".cxx");
    auto out_h_fname      = toml_config["out_h_fname"].value_or(std::string("jl") + module_name + ".h");
    auto out_report_fname = std::string("jl") + module_name + "-report.txt";


    auto veto_list = toml_config["veto_list"].value_or(""sv);

    auto auto_veto = toml_config["auto_veto"].value_or(true);

    auto propagation_mode  = toml_config["propagation_mode"].value_or("types"sv);

    auto build_cmd  = toml_config["build_cmd"].value_or("echo No build command defined"sv);

    auto test_build = toml_config["test_build"].value_or(false);

    auto build_nskips = toml_config["build_nskips"].value_or(0);
    auto build_nmax = toml_config["build_nmax"].value_or(-1);
    auto build_every = toml_config["build_every"].value_or(1);

    auto fields_and_variables = toml_config["fields_an_variables"].value_or(true);

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


    auto open_mode = std::ofstream::out;
    if(options.count("force") == 0){
      open_mode |= std::ofstream::app;
    }



    bool in_err = false;

    auto open_file = [&](const std::string& fname){
      std::ofstream f(fname, open_mode);
      if(f.tellp()!=0){
        std::cerr << "File " << fname << " is on way, please move it or use the --force option to force its deletion.\n";
        in_err = true;
      }
      return f;
    };

    //    std::ofstream out_cpp(out_cpp_fname, open_mode );
    //    if(out_cpp.tellp()!=0){
    //      std::cerr << "File " << out_cpp_fname << " is on way, please move it or use the --force option to force its deletion.\n";
    //      in_err = true;
    //    }
    auto out_cpp = open_file(out_cpp_fname);

    //    std::ofstream out_h(out_h_fname, open_mode );
    //    if(out_h.tellp()!=0){
    //      std::cerr << "File " << out_h_fname << " is on way, please move it or use the --force option to force its deletion.\n";
    //      in_err = true;
    //    }
    auto out_h = open_file(out_h_fname);

    //    std::ofstream out_jl(out_jl_fname, open_mode);
    //    if(out_jl.tellp()!=0){
    //      std::cerr << "File " << out_jl_fname << " is on way, please move it or use the --force option to force its deletion.\n";
    //      in_err = true;
    //    }
    auto out_jl = open_file(out_jl_fname);

    //    std::ofstream out_export_jl(out_export_jl_fname, open_mode);
    //    if(out_export_jl.tellp()!=0){
    //      std::cerr << "File " << out_export_jl_fname << " is on way, please move it or use the --force option to force its deletion.\n";
    //      in_err = true;
    //    }
    std::ofstream out_export_jl_;
    bool same_ = true;
    if(out_export_jl_fname.size() > 0){
      same_ = false;
      out_export_jl_ = std::move(open_file(out_export_jl_fname));
    }
    auto& out_export_jl = same_ ? out_jl : out_export_jl_;

    std::ofstream out_report(out_report_fname, open_mode);
    if(out_report.tellp()!=0){
      std::cerr << "File " << out_report_fname << " is on way, please move it or use the --force option to force its deletion.\n";
      in_err = true;
    }

    if(in_err) return -1;

    verbose = verbosity;

    CodeTree tree;

    tree.add_std_option(cxx_std);

    tree.auto_veto(auto_veto);
    tree.enableTestBuild(test_build);
    tree.build_nskips(build_nskips);
    tree.build_nmax(build_nmax);
    tree.build_every(build_every);
    tree.build_cmd(std::string(build_cmd));
    tree.inheritances(inheritances);
    tree.vetoed_finalizer_classes(vetoed_finalizer_classes);
    tree.accessor_generation_enabled(fields_and_variables);

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

    tree.parse(out_h, out_h_fname);
    tree.preprocess();
    tree.generate_cxx(out_cpp);
    tree.generate_jl(out_jl, out_export_jl, module_name, lib_basename);

    tree.report(out_report);
  }
}
