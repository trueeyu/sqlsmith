#include <algorithm>
#include <stdexcept>
#include <cassert>

#include "random.hh"
#include "relmodel.hh"
#include "grammar.hh"
#include "impedance.hh"

using namespace std;

shared_ptr<table_ref> table_ref::factory(prod *p) {
    try {
        if (p->level < 3 + d6()) {
            if (d6() > 3 && p->level < d6())
                return make_shared<table_subquery>(p);
            if (d6() > 3)
                return make_shared<joined_table>(p);
        }
        return make_shared<table_or_query_name>(p);
    } catch (runtime_error &e) {
        p->retry();
    }
    return factory(p);
}

std::shared_ptr<table_ref> table_or_query_name::factory(prod *p) {
    try {
        return make_shared<table_or_query_name>(p);
    } catch (runtime_error& e) {
        p->retry();
    }
    return factory(p);
}

table_or_query_name::table_or_query_name(prod *p) : table_ref(p) {
    t = random_pick(scope->tables);
    refs.push_back(make_shared<aliased_relation>(scope->stmt_uid("ref"), t));
}

void table_or_query_name::out(std::ostream &out) {
    out << t->ident() << " as " << refs[0]->ident();
}

table_subquery::table_subquery(prod *p)
        : table_ref(p) {
    query = make_shared<query_spec>(this, scope);
    string alias = scope->stmt_uid("subq");
    relation *aliased_rel = &query->select_list->derived_table;
    refs.push_back(make_shared<aliased_relation>(alias, aliased_rel));
}

table_subquery::~table_subquery() = default;

void table_subquery::accept(prod_visitor *v) {
    query->accept(v);
    v->visit(this);
}

shared_ptr<join_cond> join_cond::factory(prod *p, table_ref &lhs, table_ref &rhs) {
    try {
        //TODO: tmp comment
        //if (d6() < 6)
        //    return make_shared<expr_join_cond>(p, lhs, rhs);
        //else
            return make_shared<simple_join_cond>(p, lhs, rhs);
    } catch (runtime_error &e) {
        p->retry();
    }
    return factory(p, lhs, rhs);
}

simple_join_cond::simple_join_cond(prod *p, table_ref &lhs, table_ref &rhs)
        : join_cond(p, lhs, rhs) {
    retry:
    named_relation *left_rel = &*random_pick(lhs.refs);

    if (left_rel->columns().empty()) {
        retry();
        goto retry;
    }

    named_relation *right_rel = &*random_pick(rhs.refs);

    column &c1 = random_pick(left_rel->columns());

    for (const auto &c2 : right_rel->columns()) {
        if (c1.type == c2.type) {
            condition +=
                    left_rel->ident() + "." + c1.name + " = " + right_rel->ident() + "." + c2.name + " ";
            break;
        }
    }
    if (condition.empty()) {
        retry();
        goto retry;
    }
}

void simple_join_cond::out(std::ostream &out) {
    out << condition;
}

expr_join_cond::expr_join_cond(prod *p, table_ref &lhs, table_ref &rhs)
        : join_cond(p, lhs, rhs), joinscope(p->scope) {
    scope = &joinscope;
    for (const auto &ref: lhs.refs)
        joinscope.refs.push_back(&*ref);
    for (const auto &ref: rhs.refs)
        joinscope.refs.push_back(&*ref);
    search = bool_expr::factory(this);
}

void expr_join_cond::out(std::ostream &out) {
    out << *search;
}

joined_table::joined_table(prod *p) : table_ref(p) {
    lhs = table_ref::factory(this);
    rhs = table_or_query_name::factory(this);
    // TODO: tmp
    //rhs = table_ref::factory(this);

    condition = join_cond::factory(this, *lhs, *rhs);

    if (d6() < 4) {
        type = "inner";
    } else if (d6() < 4) {
        type = "left";
    } else {
        type = "right";
    }

    for (const auto &ref: lhs->refs)
        refs.push_back(ref);
    for (const auto &ref: rhs->refs)
        refs.push_back(ref);
}

void joined_table::out(std::ostream &out) {
    out << *lhs;
    indent(out);
    out << type << " join " << *rhs;
    indent(out);
    out << "on (" << *condition << ")";
}

void table_subquery::out(std::ostream &out) {
    out << "(" << *query << ") as " << refs[0]->ident();
}

void from_clause::out(std::ostream &out) {
    if (reflist.empty())
        return;
    out << "from ";

    for (auto r = reflist.begin(); r < reflist.end(); r++) {
        indent(out);
        out << **r;
        if (r + 1 != reflist.end())
            out << ",";
    }
}

from_clause::from_clause(prod *p) : prod(p) {
    reflist.push_back(table_ref::factory(this));
    for (const auto &r : reflist.back()->refs)
        scope->refs.push_back(&*r);
}

select_list::select_list(prod *p) : prod(p) {
    do {
        shared_ptr<value_expr> e = value_expr::factory(this);
        value_exprs.push_back(e);
        ostringstream name;
        name << "c" << columns++;
        sqltype *t = e->type;
        assert(t);
        derived_table.columns().emplace_back(name.str(), t);
    } while (d6() > 1);
}

void select_list::out(std::ostream &out) {
    int i = 0;
    for (auto expr = value_exprs.begin(); expr != value_exprs.end(); expr++) {
        indent(out);
        out << **expr << " as " << derived_table.columns()[i].name;
        i++;
        if (expr + 1 != value_exprs.end())
            out << ", ";
    }
}

void query_spec::out(std::ostream &out) {
    out << "select " << set_quantifier << " "
        << *select_list;
    indent(out);
    out << *from_clause;
    indent(out);
    out << "where ";
    out << *search;
    if (limit_clause.length()) {
        indent(out);
        out << limit_clause;
    }
}

query_spec::query_spec(prod *p, struct scope *s) :
        prod(p), myscope(s) {
    scope = &myscope;
    scope->tables = s->tables;

    from_clause = make_shared<struct from_clause>(this);
    select_list = make_shared<struct select_list>(this);

    set_quantifier = (d100() == 1) ? "distinct" : "";

    search = bool_expr::factory(this);

    if (d6() > 2) {
        ostringstream cons;
        cons << "limit " << d100() + d100();
        limit_clause = cons.str();
    }
}

shared_ptr<prod> statement_factory(struct scope *s) {
    try {
        s->new_stmt();
        return make_shared<query_spec>((struct prod *) nullptr, s);
    } catch (runtime_error &e) {
        return statement_factory(s);
    }
}

