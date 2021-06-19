#include "config.h"
#include <iostream>

#include <regex>
#include <string>

#include "log.hh"
#include "schema.hh"
#include "gitrev.h"
#include "impedance.hh"
#include "random.hh"

using namespace std;

struct stats_visitor : prod_visitor {
    int nodes = 0;
    int maxlevel = 0;
    long retries = 0;
    map<const char *, long> production_stats;

    void visit(struct prod *p) override {
        nodes++;
        if (p->level > maxlevel)
            maxlevel = p->level;
        production_stats[typeid(*p).name()]++;
        retries += p->retries;
    }
};

void stats_collecting_logger::generated(prod &query) {
    queries++;

    stats_visitor v;
    query.accept(&v);

    sum_nodes += (float) (v.nodes);
    sum_height += (float) (v.maxlevel);
    sum_retries += (float) (v.retries);
}

void cerr_logger::report() {
    cerr << endl << "queries: " << queries << endl;
    cerr << "AST stats (avg): height = " << sum_height / (float) (queries)
         << " nodes = " << sum_nodes / (float) (queries) << endl;

    vector<pair<std::string, long> > report;
    for (const auto &e : errors) {
        report.emplace_back(e);
    }
    stable_sort(report.begin(), report.end(),
                [](const pair<std::string, long> &a,
                   const pair<std::string, long> &b) { return a.second > b.second; });
    long err_count = 0;
    for (const auto &e : report) {
        err_count += e.second;
        cerr << e.second << "\t" << e.first.substr(0, 80) << endl;
    }
    cerr << "error rate: " << (float) err_count / (float) (queries) << endl;
    impedance::report();
}


void cerr_logger::generated(prod &p) {
    stats_collecting_logger::generated(p);
    if ((10 * columns - 1) == queries % (10 * columns))
        report();
}

void cerr_logger::executed(prod &query) {
    (void) query;
    if (columns - 1 == (queries % columns)) {
        cerr << endl;
    }
    cerr << ".";
}

void cerr_logger::error(prod &query, const dut::failure &e) {
    (void) query;
    istringstream err(e.what());
    string line;

    if (columns - 1 == (queries % columns)) {
        cerr << endl;
    }
    getline(err, line);
    errors[line]++;
    if (dynamic_cast<const dut::timeout *>(&e))
        cerr << "t";
    else if (dynamic_cast<const dut::syntax *>(&e))
        cerr << "S";
    else if (dynamic_cast<const dut::broken *>(&e))
        cerr << "C";
    else
        cerr << "e";
}

