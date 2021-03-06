#ifndef CODETREE_H
#define CODETREE_H

// Copyright (C) 2021 Philippe Gras <philippe.gras@cern.ch>

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

//#include "Entity.h"
#include "TypeRcd.h"
#include "MethodRcd.h"

#include <clang-c/Index.h>

#include "utils.h"

namespace fs = std::filesystem;


namespace codetree{

  enum class propagation_mode_t { types, methods };
  enum class export_mode_t { none, member_functions, all_functions, all };
  
  class CodeTree{
  public:
    CodeTree(): include_depth_(1), mainFileOnly_(true), override_base_(false),
		propagation_mode_(propagation_mode_t::types), export_mode_(export_mode_t::member_functions),
		unit_(nullptr), index_(nullptr),
		build_cmd_("echo Build command not defined."),  test_build_(false),
		ibuild_(0), build_nskips_(0), build_nmax_(-1), build_every_(1),
		visiting_a_templated_class_(false), accessor_generation_enabled_(false)
    {
      opts_.push_back("-x");
      opts_.push_back("c++-header");
    }
    CodeTree(CodeTree&&) = default;

    ~CodeTree();
    
    std::vector<std::string> include_files;
    
    std::vector<TypeRcd> types_;
    std::vector<MethodRcd> functions_;
    std::vector<TypeRcd> enums_;
    std::vector<CXCursor> typedefs_;
    std::vector<CXCursor> vars_;
    std::set<std::string> no_mirror_types_;
    std::vector<CXCursor> visited_classes_;
    int include_depth_;
    std::vector<std::string> files_to_wrap_;
    std::vector<std::string> files_to_wrap_fullpaths_;

    std::string build_cmd_;
    bool test_build_;
    int ibuild_;
    int build_nskips_;
    int build_nmax_;
    int build_every_;

    bool accessor_generation_enabled() const { return accessor_generation_enabled_;}

    void accessor_generation_enabled(bool val) { accessor_generation_enabled_ = val;}
    
    void export_mode(export_mode_t mode){ export_mode_ = mode; }

    void add_source_file(const fs::path& fname){
      files_to_wrap_.push_back(fname.c_str());
    }
    
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
    
    bool fromMainFiles(const CXCursor& cursor) const;

    std::ostream& generate_cxx(std::ostream&);

    std::ostream& generate_default_ctor_cxx(std::ostream&o, const TypeRcd& type) const;

    
    std::ostream& generate_enum_cxx(std::ostream& o, CXCursor cursor);

    
    //To be called before the generate_xx functions.
    //Sorts the types such that a parent type appears in the list
    //before its children
    void preprocess();

    void add_forced_header(const std::string& header){ forced_headers_.push_back(header);}

    const std::vector<std::string>& forced_headers() { return forced_headers_; }

    void parse(std::ofstream& header_file, const std::filesystem::path& header_file_path);

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
    generate_templated_type_cxx(std::ostream& o, const TypeRcd& type_rcd);

    std::ostream&
    generate_methods_of_templated_type_cxx(std::ostream& o, const TypeRcd& t);

    void inheritances(const std::vector<std::string>& val);

    void build_cmd(const std::string& val){ build_cmd_ = val; }

    void enableTestBuild(bool val){ test_build_ = val; }

    void build_nskips(int val){ build_nskips_ = val;}
    
    void build_nmax(int val){ build_nmax_ = val;}

    void build_every(int val){ build_every_ = val;}

    void add_export_veto_word(const std::string& s) { export_blacklist_.insert(s); }


  protected:

    enum class accessor_mode_t {none, getter, both };

    bool in_veto_list(const std::string signature) const;
    
    //Check is a field or variable is veto status for accessor generation
    // acccessor_mode_t::none -> both accessors vetoed
    accessor_mode_t
    check_veto_list_for_var_or_field(const CXCursor& cursor, bool global_var) const;
    
    //Look for a file in the include dirs and returns the path
    std::string resolve_include_path(const std::string& fname);
    
    //Runs the wrapper build command
    void test_build(std::ostream& o);
    
    bool has_an_implicit_default_ctor(const CXCursor& type_cursor) const;
    
    CXToken* next_token(CXSourceLocation loc) const;
    CXSourceRange function_decl_range(const CXCursor& cursor) const;
    
    bool add_type_specialization(TypeRcd* pTypeRcd,  const CXType& type);
    
    //Finds the definition of a type of the underlying type in case
    //of a pointer or reference. In case of a templated type,
    //it retrieves also the types of the template parameters.
    std::tuple<bool, std::vector<CXCursor>>
    find_type_definition(const CXType& type) const;

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

    std::ostream& generate_type_cxx(std::ostream& o, const TypeRcd& cursor);

    std::ostream&
    generate_accessor_cxx(std::ostream& o, const TypeRcd* type_rcd, const CXCursor& cursor, bool getter_only);

    
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
    visit_class(CXCursor cursor, std::string type_name = std::string());

    void
    visit_member_function(CXCursor cursor);

    void
    visit_class_constructor(CXCursor cursor);

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
    ///a declaration
    bool register_type(const CXType& type);

    int get_min_required_args(CXCursor cursor) const;

  private:
    bool auto_veto_;
    
    fs::path filename_;

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

    //Map of child->mother class inheritance preference
    std::map<std::string, std::string> inheritance_;

    bool visiting_a_templated_class_;

    std::vector<std::string> include_dirs_;

    std::set<std::string> export_blacklist_;

    bool accessor_generation_enabled_;
  };
}
#endif //CODETREE_H not defined
