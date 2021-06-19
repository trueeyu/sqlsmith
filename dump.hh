/// @file
/// @brief Dump syntax trees as GraphML
#ifndef DUMP_HH
#define DUMP_HH

#include <iostream>
#include <fstream>
#include <string>

#include "prod.hh"
#include "log.hh"

struct graphml_dumper : prod_visitor {
  std::ostream &o;
  void visit(struct prod *p) override;
  explicit graphml_dumper(std::ostream &out);
  static std::string id(prod *p);
  ~graphml_dumper() override;
};

struct ast_logger : logger {
  int queries = 0;
  void generated(prod &query) override;
};

#endif
