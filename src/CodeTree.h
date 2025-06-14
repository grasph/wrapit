//-*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// vim: noai:ts=2:sw=2:expandtab
//
// Copyright (C) 2021 Philippe Gras <philippe.gras@cern.ch>
//
#ifndef CODETREE_H
#define CODETREE_H

#include <vector>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <regex>
#include <iostream>
#include <filesystem>
#include <tuple>
#include <map>

//#include "Entity.h"
#include "TypeRcd.h"
#include "MethodRcd.h"

#include <clang-c/Index.h>

#include "utils.h"
#include "config.h"

#include "TypeMapper.h"
#include "Graph.h"

//to be used by set<CXCursor>
static bool operator<(const CXCursor& c1, const CXCursor& c2){
  return c1.data[0] < c2.data[0]
    || (c1.data[0] == c2.data[0] && c1.data[1] < c2.data[1])
    || (c1.data[0] == c2.data[0] && c1.data[1] == c2.data[1] && c1.data[2] < c2.data[2]);
}

namespace fs = std::filesystem;


namespace codetree{

  enum class propagation_mode_t { types, methods };
  enum class export_mode_t { none, member_functions, all_functions, all };

  class CodeTree{
  public:
    CodeTree(): module_name_("Module"),
                out_open_mode_(std::ios_base::app),
                out_cxx_dir_("src"),
                auto_veto_(true), update_mode_(true), include_depth_(1),
                mainFileOnly_(true), override_base_(false),
                propagation_mode_(propagation_mode_t::types),
                export_mode_(export_mode_t::member_functions),
                unit_(nullptr), index_(nullptr), n_classes_per_file_(-1),
                build_cmd_("echo Build command not defined."),
                test_build_(false), ibuild_(0), build_nskips_(0),
                build_nmax_(-1), build_every_(1),
                visiting_a_templated_class_(false),
                accessor_generation_enabled_(false),
                import_getindex_(false),
                import_setindex_(false)
    {
      opts_.push_back("-x");
      opts_.push_back("c++-header");
      type_map_.add("std::string_view", "const char *", "std::string");
      type_map_.add("const std::string_view &", "const char *", "std::string");
      type_map_.add("const char *", "const char *", "std::string");
      set_clang_resource_dir(CLANG_RESOURCE_DIR);
    }
    CodeTree(CodeTree&&) = default;

    ~CodeTree();

    std::vector<std::string> include_files;

    std::vector<TypeRcd> types_;
    std::vector<unsigned> types_sorted_indices_;
    std::vector<MethodRcd> functions_;
    std::vector<TypeRcd> enums_;
    std::vector<CXCursor> typedefs_;
    std::vector<CXCursor> vars_;
    std::set<std::string> no_mirror_types_;
    std::vector<CXCursor> visited_classes_;
    int include_depth_;
    std::vector<std::string> files_to_wrap_;
    std::vector<std::string> files_to_wrap_fullpaths_;
    std::vector<std::string> wrapped_methods_;

    int n_classes_per_file_;
    std::string out_cxx_dir_;
    std::string out_jl_dir_;


    std::string build_cmd_;
    bool test_build_;
    int ibuild_;
    int build_nskips_;
    int build_nmax_;
    int build_every_;

    void auto_veto(bool val){ auto_veto_ = val; }
    bool auto_veto() { return auto_veto_; }

    //if not empty a file with this created to be included
    //in CMake build base configuration files
    void cmake(const std::string& val){ cmake_ = val; }
    std::string cmake() { return cmake_; }

    bool accessor_generation_enabled() const { return accessor_generation_enabled_;}

    void accessor_generation_enabled(bool val) { accessor_generation_enabled_ = val;}

    void export_mode(export_mode_t mode){ export_mode_ = mode; }

    void add_source_file(const fs::path& fname){
      files_to_wrap_.push_back(fname.string());
    }

    static std::string resolve_clang_resource_dir_path(std::string relpath);

    bool has_cursor(const std::vector<CXCursor> vec, const CXCursor& cursor){
      for(const auto& c: vec){
        if(clang_equalCursors(c, cursor)) return true;
      }
      return false;
    }

    bool has_cursor(const std::vector<TypeRcd> vec, const CXCursor& cursor){
      for(const auto& e: vec){
        if(clang_equalCursors(e.cursor, cursor)) return true;
      }
      return false;
    }

    bool has_type_name(const std::vector<TypeRcd> vec, const std::string& type_name){
      for(const auto& e: vec){
        if(e.type_name == type_name) return true;
      }
      return false;
    }

    bool has_file(const std::vector<CXFile> vec, const CXFile& file) const{
      for(const auto& f: vec){
        if(clang_File_isEqual(f, file)) return true;
      }
      return false;
    }

    bool isAccessible(CXType type) const;

    CXType resolve_private_typedef(CXType type) const;

    static std::string libclangdir();

    //Set clang resource directory.
    //Relative paths are relative to the directory containing
    //the libclang.so library the program is linked to.
    void set_clang_resource_dir(std::string path){
      clang_resource_dir_ = resolve_clang_resource_dir_path(path);
    }

    void set_update_mode(bool val){
      update_mode_ = true;
    }

    bool fromMainFiles(const CXCursor& cursor) const;

    std::string wrapper_classsname(const std::string& classname) const;

    std::ostream& generate_template_add_type_cxx(std::ostream& o,
                                                 const TypeRcd& type_rcd,
                                                 std::string& add_type_param);

    std::ostream& generate_cxx_for_type(std::ostream& o,
                                        const TypeRcd& t);

    std::ostream& generate_non_template_add_type_cxx(std::ostream& o,
                                                     const TypeRcd& type_rcd,
                                                     std::string&  add_type_param);

    //the preprocess method must be called before invoking this function
    void generate_cxx();

    std::ostream& generate_enum_cxx(std::ostream& o, CXCursor cursor);

    void generate_project_file(std::ostream&o,
                               const std::string& uuid,
                               const std::string& version);


    //To be called before the generate_xx functions.
    //Sorts the types such that a parent type appears in the list
    //before its children
    void preprocess();

    void add_forced_header(const std::string& header){ forced_headers_.push_back(header);}

    const std::vector<std::string>& forced_headers() { return forced_headers_; }

    bool parse();

    void parse_vetoes(const fs::path& fname);

    std::ostream& generate_jl(std::ostream& o,
                              std::ostream& export_o,
                              const std::string& module_name,
                              const std::string& shared_lib_name) const;

    std::ostream& report(std::ostream& o);

    void add_std_option(const std::string x){
      opts_.push_back(std::string("-std=") + x);
    }

    void add_include_dir(const std::string& x){
      include_dirs_.push_back(x);
      opts_.emplace_back("-I");
      opts_.push_back(x);
    }

    void define_macro(const std::string& x){
      opts_.emplace_back("-D");
      opts_.push_back(x);
    }

    void enable_feature(const std::string& x){
      opts_.emplace_back("-x");
      opts_.push_back(x);
    }

    void add_clang_opt(const std::string& x){
      opts_.push_back(x);
    }

    void propagation_mode(propagation_mode_t val){
      propagation_mode_ = val;
    }


    std::ostream&
    generate_methods_of_templated_type_cxx(std::ostream& o, const TypeRcd& t);

    void inheritances(const std::vector<std::string>& val);

    void vetoed_finalizer_classes(const std::vector<std::string>& val){
      finalizers_to_veto_ = val;
    }

    void vetoed_copy_ctor_classes(const std::vector<std::string>& val){
      for(const auto& k: val) copy_ctor_to_veto_.insert(k);
    }


    void build_cmd(const std::string& val){ build_cmd_ = val; }

    void enableTestBuild(bool val){ test_build_ = val; }

    void build_nskips(int val){ build_nskips_ = val;}

    void build_nmax(int val){ build_nmax_ = val;}

    void build_every(int val){ build_every_ = val;}

    void add_export_veto_word(const std::string& s) { export_blacklist_.insert(s); }

    //Sets version of CxxWrap the code should be generated for.
    //Version is coded as
    //10^6 * major_number + 1000 * minor_number +  patch_number;
    void set_cxxwrap_version(long val){ cxxwrap_version_ = val; }

    void set_n_classes_per_file(int n_classes_per_file) { n_classes_per_file_ = n_classes_per_file; }

    void set_out_cxx_dir(const std::string& val) { out_cxx_dir_ = val; }

    void set_out_jl_dir(const std::string& val) { out_jl_dir_ = val; }

    void set_module_name(const std::string& val){ module_name_ = val;}

    void set_force_mode(bool forced){ out_open_mode_ = forced ? std::ios_base::out : std::ios_base::app; }

    void set_julia_names(const std::vector<std::string>& name_map);

    void set_class_order_constraints(const std::vector<std::string>& class_order_constraints);
    
    void set_mapped_types(const std::vector<std::string>& name_map);

  protected:

    enum class accessor_mode_t {none, getter, both };

    // Adds a type to the list of types/classes to wrap.
    // Returns the index of the added type in the vector types_
    int add_type(const CXCursor& cursor, bool check = true);

    // Checks if type is a std::vector or a std::valarray
    // and if it is the case, marks the element type as requiring
    // std::vector and std:valarray support.
    void check_for_stl(const CXType& type);

    bool in_veto_list(const std::string signature) const;

    //Check is a field or variable is veto status for accessor generation
    // acccessor_mode_t::none -> both accessors vetoed
    accessor_mode_t
    check_veto_list_for_var_or_field(const CXCursor& cursor, bool global_var) const;

    //Look for a file in the include dirs and returns the path
    std::string resolve_include_path(const std::string& fname);

    //Runs the wrapper build command
    void test_build(std::ostream& o);

    void set_type_rcd_ctor_info(TypeRcd& rcd);

    CXToken* next_token(CXSourceLocation loc) const;
    CXSourceRange function_decl_range(const CXCursor& cursor) const;

    bool add_type_specialization(TypeRcd* pTypeRcd, const CXType& type);

    bool check_resource_dir(bool verbose) const;

    //Finds the definition of a type or the underlying type in case
    //of a pointer or reference. For a templated type,
    //it retrieves also the types of the template parameters.
    std::tuple<bool, std::vector<CXCursor>>
    find_type_definition(const CXType& type) const;

    bool requires_default_ctor_wrapper(const TypeRcd& type_rcd) const;

    //Used by find_type_definition.
    std::tuple<bool, CXCursor> find_base_type_definition_(const CXType& type0) const;

    bool is_method_deleted(CXCursor cursor) const;

    std::string getUnQualifiedTypeSpelling(const CXType& type) const;

    bool inform_missing_types(std::vector<CXType> missing_types,
                              const MethodRcd& methodRcd,
                              const TypeRcd* classRcd = nullptr) const;


    bool is_type_vetoed(const std::string& type_name) const;

    static CXChildVisitResult visit(CXCursor cursor, CXCursor parent, CXClientData client_data);

    CXCursor getParentClassForWrapper(CXCursor cursor) const;

    std::ostream&
    generate_accessor_cxx(std::ostream& o, const TypeRcd* type_rcd,
                          const CXCursor& cursor, bool getter_only, int nindents);


    std::string type_name(CXCursor cursor) const{
      if(clang_Cursor_isNull(cursor)) return std::string();

      auto type = clang_getCursorType(cursor);

      if(type.kind == CXType_Invalid) return std::string();

      auto s = clang_getTypeSpelling(type);
      std::string r(clang_getCString(s));
      //FIXME
      //      clang_disposeString(s);
      return r;
    }


    bool is_to_visit(CXCursor cursor) const;

    TypeRcd* find_class_of_method(const CXCursor& method);

    void
    visit_class(CXCursor cursor);

    void
    visit_member_function(CXCursor cursor);

    void
    visit_class_constructor(CXCursor cursor);

    void
    visit_class_destructor(CXCursor cursor);

    void
    visit_enum(CXCursor cursor);

    void
    visit_class_template_specialization(CXCursor cursor);

    void
    visit_global_function(CXCursor cursor);

    void
    visit_field_or_global_variable(CXCursor cursor);

    void
    visit_typedef(CXCursor cursor);

    std::set<std::string> types_missing_def_;
    std::set<std::string> builtin_types_;
    std::set<CXCursor> auto_vetoed_methods_;

    std::vector<std::string> forced_headers_;

    std::vector<std::string> opts_;

    std::vector<std::string> get_enum_constants(CXCursor cursor) const;

    std::ostream& method_cxx_decl(std::ostream& o, const MethodRcd& method,
                                  std::string varname = "",
                                  std::string classname = "",
                                  int nindents = 1,
                                  bool templated = false);

    std::ostream& generate_method_cxx(std::ostream& o, const MethodRcd& method);

    std::string get_prefix(const std::string& type_name) const;

    void disable_owner_mirror(CXCursor cursor);

    std::tuple<std::vector<CXType>, int>
    visit_function_arg_and_return_types(CXCursor cursor);

    std::string fundamental_type(std::string type);

    bool has_type(CXCursor cursor) const;

    bool has_type(const std::string& t) const;

    bool isAForwardDeclaration(CXCursor cursor) const;


    bool arg_with_default_value(const CXCursor& c) const;


    ///Register a type. Returns true if the type
    ///definition is known, false if we have only
    ///a declaration.
    ///if pItype (resp. pIenum) is not null and a type was added to types_
    ///(resp. enums_) assign the index to *pItype (resp. pIenum).
    bool register_type(const CXType& type,
                       int* pItype = nullptr,
                       int* pIenum = nullptr);

    int get_min_required_args(CXCursor cursor) const;

    std::ostream& show_stats(std::ostream& o) const;

    void exit_if_wrapper_files_in_the_way();

    //the types_sorted_indices_ must be set and up to date
    //before calling this function
    void update_wrapper_filenames();

    std::ostream& generate_version_check_cxx(std::ostream& o) const;

    std::ostream& generate_type_wrapper_header(std::ostream& o) const;

    //Try to open file path for writing. Exit application if
    //either an existing file is in the way and the force mode is disabled
    //or the file opening failed.
    std::ofstream checked_open(const std::string& path) const;

    std::ostream&
    gen_apply_stl(std::ostream& o, int indent_depth,
                  const TypeRcd& type_rcd,
                  const std::string& add_type_param) const;


    bool is_natively_supported(const CXType& type, int* params = nullptr) const;

    bool is_natively_supported(const std::string& type_fqn,
                               int* nparams = nullptr) const;


  private:
    std::string clang_resource_dir_;

    bool auto_veto_;

    std::string cmake_;

    long cxxwrap_version_;

    bool update_mode_;

    std::string header_file_path_;

    std::map<std::string, std::string> cxx_to_julia_;

    std::map<std::string, std::string> type_straight_mapping_;

    std::string module_name_;

    std::ios_base::openmode out_open_mode_;

    bool mainFileOnly_;

    std::vector<std::string> veto_list_;

    bool override_base_;

    std::vector<std::string> get_index_generated_;

    std::vector<unsigned> incomplete_types_;

    propagation_mode_t propagation_mode_;

    std::set<std::string> to_export_;
    export_mode_t export_mode_;


    CXTranslationUnit unit_;
    CXIndex index_;

    //Current top-level visited cursor
    CXCursor visited_cursor_;
    
    //Map of child->mother class inheritance preference
    std::map<std::string, std::string> inheritance_;

    //List of classes whose finalizer must be disabled
    std::vector<std::string> finalizers_to_veto_;

    //List of classes whose finalizer has been disabled
    std::set<std::string> vetoed_finalizers_;

    //List of classes whose wrapping of copy constructor has been disabled by configuration
    std::set<std::string> copy_ctor_to_veto_;

    bool visiting_a_templated_class_;

    std::vector<std::string> include_dirs_;

    std::set<std::string> export_blacklist_;

    bool accessor_generation_enabled_;

    bool import_getindex_;

    bool import_setindex_;

    Graph type_dependencies_;

    std::vector<std::pair<std::string, std::string>> class_order_constraints_;
    
    std::vector<std::string> towrap_type_filenames_;
    std::set<std::string> towrap_type_filenames_set_;

    TypeMapper type_map_;

    struct {
      unsigned enums = 0;
      unsigned types = 0;
      unsigned type_templates = 0;
      unsigned methods = 0;
      unsigned field_getters = 0;
      unsigned field_setters = 0;
      unsigned global_var_getters = 0;
      unsigned global_var_setters = 0;
      unsigned global_funcs = 0;
    } nwraps_;
    
  };
}
#endif //CODETREE_H not defined
