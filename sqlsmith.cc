#include "config.h"

#include <iostream>
#include <chrono>

#include <regex>

#include <thread>
#include <typeinfo>

#include "random.hh"
#include "grammar.hh"
#include "relmodel.hh"
#include "schema.hh"
#include "gitrev.h"

#include "log.hh"
#include "dump.hh"
#include "impedance.hh"
#include "dut.hh"

#include "mysql.hh"
#include <cstdlib>
#include <csignal>
#include <unistd.h>

using namespace std;

using namespace std::chrono;

/* make the cerr logger globally accessible so we can emit one last report on SIGINT */
cerr_logger *global_cerr_logger;

extern "C" void cerr_log_handler(int) {
    if (global_cerr_logger)
        global_cerr_logger->report();
    exit(1);
}

int main(int argc, char *argv[]) {
    map<string, string> options;
    regex optregex(
            "--(help|verbose|target|mysql|version|dump-all-graphs|dump-all-queries|seed|dry-run|max-queries|rng-state|exclude-catalog)(?:=((?:.|\n)*))?");

    for (char **opt = argv + 1; opt < argv + argc; opt++) {
        smatch match;
        string s(*opt);
        if (regex_match(s, match, optregex)) {
            options[string(match[1])] = match[2];
        } else {
            cerr << "Cannot parse option: " << *opt << endl;
            options["help"] = "";
        }
    }

    if (options.count("help")) {
        cerr <<
             "    --target=connstr     postgres database to send queries to" << endl <<
             "    --mysql=URI    		MySQL database to send queries to" << endl <<
             "    --seed=int           seed RNG with specified int instead of PID" << endl <<
             "    --dump-all-queries   print queries as they are generated" << endl <<
             "    --dump-all-graphs    dump generated ASTs" << endl <<
             "    --dry-run            print queries instead of executing them" << endl <<
             "    --exclude-catalog    don't generate queries using catalog relations" << endl <<
             "    --max-queries=long   terminate after generating this many queries" << endl <<
             "    --rng-state=string    deserialize dumped rng state" << endl <<
             "    --verbose            emit progress output" << endl <<
             "    --version            print version information and exit" << endl <<
             "    --help               print available command line options and exit" << endl;
        return 0;
    } else if (options.count("version")) {
        return 0;
    }

    try {
        shared_ptr<schema> schema = make_shared<schema_mysql>(options["mysql"], options.count("exclude-catalog"));

        scope scope;
        long queries_generated = 0;
        schema->fill_scope(scope);

        if (options.count("rng-state")) {
            istringstream(options["rng-state"]) >> smith::rng;
        } else {
            smith::rng.seed(options.count("seed") ? stoi(options["seed"]) : getpid());
        }

        vector<shared_ptr<logger> > loggers;

        loggers.push_back(make_shared<impedance_feedback>());

        if (options.count("verbose")) {
            auto l = make_shared<cerr_logger>();
            global_cerr_logger = &*l;
            loggers.push_back(l);
            signal(SIGINT, cerr_log_handler);
        }

        if (options.count("dump-all-graphs"))
            loggers.push_back(make_shared<ast_logger>());

        if (options.count("dump-all-queries"))
            loggers.push_back(make_shared<query_dumper>());

        if (options.count("dry-run")) {
            while (true) {
                shared_ptr<prod> gen = statement_factory(&scope);
                gen->out(cout);
                for (const auto &l : loggers)
                    l->generated(*gen);
                cout << ";" << endl;
                queries_generated++;

                if (options.count("max-queries")
                    && (queries_generated >= stol(options["max-queries"])))
                    return 0;
            }
        }

        shared_ptr<dut_base> dut;
        dut = make_shared<dut_mysql>(options["mysql"]);

        while (true) /* Loop to recover connection loss */
        {
            try {
                while (true) { /* Main loop */

                    if (options.count("max-queries")
                        && (++queries_generated > stol(options["max-queries"]))) {
                        if (global_cerr_logger)
                            global_cerr_logger->report();
                        return 0;
                    }

                    /* Invoke top-level production to generate AST */
                    shared_ptr<prod> gen = statement_factory(&scope);

                    for (const auto &l : loggers)
                        l->generated(*gen);

                    /* Generate SQL from AST */
                    ostringstream s;
                    gen->out(s);

                    /* Try to execute it */
                    try {
                        dut->test(s.str());
                        for (const auto &l : loggers)
                            l->executed(*gen);
                    } catch (const dut::failure &e) {
                        for (const auto &l : loggers)
                            try {
                                l->error(*gen, e);
                            } catch (runtime_error &e) {
                                cerr << endl << "log failed: " << typeid(*l).name() << ": "
                                     << e.what() << endl;
                            }
                        if ((dynamic_cast<const dut::broken *>(&e))) {
                            /* re-throw to outer loop to recover session. */
                            throw;
                        }
                    }
                }
            }
            catch (const dut::broken &e) {
                /* Give server some time to recover. */
                this_thread::sleep_for(milliseconds(1000));
            }
        }
    }
    catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
}
