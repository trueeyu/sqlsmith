/// @file
/// @brief grammar: Top-level and unsorted grammar productions

#ifndef GRAMMAR_HH
#define GRAMMAR_HH

#include <ostream>
#include "relmodel.hh"
#include <memory>
#include "schema.hh"

#include "prod.hh"
#include "expr.hh"

using std::shared_ptr;

struct table_ref : prod {
    vector<shared_ptr<named_relation> > refs;

    static shared_ptr<table_ref> factory(prod *p);

    explicit table_ref(prod *p) : prod(p) {}

    virtual ~table_ref() = default;
};

struct table_or_query_name : table_ref {
    void out(std::ostream &out) override;

    // TODO: tmp
    explicit table_or_query_name(prod *p);

    static shared_ptr<table_ref> factory(prod *p);

    ~table_or_query_name() override = default;

    named_relation *t;
};

struct table_subquery : table_ref {
    void out(std::ostream &out) override;

    shared_ptr<struct query_spec> query;

    explicit table_subquery(prod *p);

    ~table_subquery() override;

    void accept(prod_visitor *v) override;
};

struct join_cond : prod {
    static shared_ptr<join_cond> factory(prod *p, table_ref &lhs, table_ref &rhs);

    join_cond(prod *p, table_ref &lhs, table_ref &rhs)
            : prod(p) {
        (void) lhs;
        (void) rhs;
    }
};

struct simple_join_cond : join_cond {
    std::string condition;

    simple_join_cond(prod *p, table_ref &lhs, table_ref &rhs);

    void out(std::ostream &out) override;
};

struct expr_join_cond : join_cond {
    struct scope joinscope;
    shared_ptr<bool_expr> search;

    expr_join_cond(prod *p, table_ref &lhs, table_ref &rhs);

    void out(std::ostream &out) override;

    void accept(prod_visitor *v) override {
        search->accept(v);
        v->visit(this);
    }
};

struct joined_table : table_ref {
    void out(std::ostream &out) override;

    explicit joined_table(prod *p);

    std::string type;

    shared_ptr<table_ref> lhs;
    shared_ptr<table_ref> rhs;
    shared_ptr<join_cond> condition;

    ~joined_table() override = default;

    void accept(prod_visitor *v) override {
        lhs->accept(v);
        rhs->accept(v);
        condition->accept(v);
        v->visit(this);
    }
};

struct from_clause : prod {
    std::vector<shared_ptr<table_ref> > reflist;

    void out(std::ostream &out) override;

    explicit from_clause(prod *p);

    ~from_clause() = default;

    void accept(prod_visitor *v) override {
        v->visit(this);
        for (const auto &p : reflist)
            p->accept(v);
    }
};

struct select_list : prod {
    std::vector<shared_ptr<value_expr> > value_exprs;
    relation derived_table;
    int columns = 0;

    explicit select_list(prod *p);

    void out(std::ostream &out) override;

    ~select_list() = default;

    void accept(prod_visitor *v) override {
        v->visit(this);
        for (const auto &p : value_exprs)
            p->accept(v);
    }
};

struct query_spec : prod {
    std::string set_quantifier;
    shared_ptr<struct from_clause> from_clause;
    shared_ptr<struct select_list> select_list;
    shared_ptr<bool_expr> search;
    std::string limit_clause;
    struct scope myscope;

    void out(std::ostream &out) override;

    query_spec(prod *p, struct scope *s);

    void accept(prod_visitor *v) override {
        v->visit(this);
        select_list->accept(v);
        from_clause->accept(v);
        search->accept(v);
    }
};

shared_ptr<prod> statement_factory(struct scope *s);

#endif
