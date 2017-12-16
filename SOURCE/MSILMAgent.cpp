/* 
 * File:   MSILMAgent.cpp
 * Author: hiroki
 * 
 * Created on October 25, 2013, 4:08 PM
 */


#include "MSILMAgent.h"

bool MSILMAgent::SYM_FLAG = false;
bool MSILMAgent::MUT_FLAG = false;
bool MSILMAgent::EXC_FLAG = false;
bool MSILMAgent::OMISSION_FLAG = false;

MSILMAgent::MSILMAgent() {
}

MSILMAgent::~MSILMAgent() {
}

MSILMAgent&
        MSILMAgent::operator=(const MSILMAgent& dst) {
    kb = dst.kb;
    generation_index = dst.generation_index;
    serial = dst.serial;
    LOGGING_FLAG = dst.LOGGING_FLAG;
    SYM_FLAG = dst.SYM_FLAG;
    MUT_FLAG = dst.MUT_FLAG;
    EXC_FLAG = dst.EXC_FLAG;
    OMISSION_FLAG = dst.OMISSION_FLAG;
    last_selected_meaning = dst.last_selected_meaning;
    return *this;
}

MSILMAgent
MSILMAgent::make_child(void) {
    MSILMAgent child;
    child.generation_index = generation_index + 1;
    child.serial = KirbyAgent::indexer.generate();
    return child;
}

MSILMAgent&
MSILMAgent::grow(std::vector<Rule> meanings) {

    kb.DIC_BLD = false;
    kb.word_dic.clear();
    kb.build_word_index();

    if (INDEXER_FLAG)
        kb.indexer(meanings);


    bool deleted_flag = false;
    if (DEL_LONG_RULE & LOGGING_FLAG) {
        LogBox::push_log("->>DELETE ON");
        LogBox::push_log(
                "->>LENGTH" + boost::lexical_cast<std::string>(DEL_LONG_RULE_LENGTH));
    }


    if (DEL_LONG_RULE) {
        for (int i = 0; i < meanings.size(); i++) {
            std::map<KnowledgeBase::PATTERN_TYPE,
                    std::vector<KnowledgeBase::PatternType> > pats, pats2;
            std::vector<KnowledgeBase::PatternType> abpat, cmpat;
            pats = kb.construct_grounding_patterns(meanings[i]);
            pats2 = kb.natural_construct_grounding_patterns(meanings[i]);
            kb.exception_filter(pats, pats2);
            abpat = pats[KnowledgeBase::ABSOLUTE];
            cmpat = pats[KnowledgeBase::COMPLETE];

            //absolute (horistic)
            for (int j = 0; j < abpat.size(); j++) {
                //if (LOGGING_FLAG) {
                //  LogBox::push_log("->>CHECK(A): " + abpat[j].front().to_s());
                //}

                if (abpat[j].front().external.size() > DEL_LONG_RULE_LENGTH) {
                    std::vector<Rule>::iterator it;
                    it = kb.sentenceDB.begin();
                    while (it != kb.sentenceDB.end()) {
                        if ((*it) == abpat[j].front()) {
                            it = kb.sentenceDB.erase(it);
                        } else {
                            it++;
                        }
                    }
                }
            }

            for (int j = 0; j < cmpat.size(); j++) {
                Rule rule, sent;
                rule.type = RULE_TYPE::SENTENCE;
                rule.internal = meanings[i].internal;
                sent = cmpat[j].front();
                kb.graund_with_pattern(rule, cmpat[j]);
                cmpat[j].push_back(sent);

                //if (LOGGING_FLAG) {
                //  LogBox::push_log("->>CHECK(C): " + rule.to_s());
                //}
                if (rule.external.size() > DEL_LONG_RULE_LENGTH) {
                    //if (LOGGING_FLAG) {
                    //  LogBox::push_log("->>DEL SELECT FOR: " + rule.to_s());
                    //}

                    Rule del;
                    del.type = RULE_TYPE::SENTENCE;
                    //del.external = std::vector<Element>();
                    for (int k = 0; k < cmpat[j].size(); k++) {
                        //if (LOGGING_FLAG) {
                        //  LogBox::push_log("->>CHECK: " + cmpat[j][k].to_s());
                        //}
                        if (del.external.size() < cmpat[j][k].external.size()) {
                            del = cmpat[j][k];
                        }
                    }

                    //if (LOGGING_FLAG) {
                    //  LogBox::push_log("->>SELECTED: " + del.to_s());
                    //}

                    std::vector<Rule>::iterator it;
                    if (del.is_sentence()) {
                        it = kb.sentenceDB.begin();
                        while (it != kb.sentenceDB.end()) {
                            //if (LOGGING_FLAG) {
                            //  LogBox::push_log("->>SEEK: " + (*it).to_s());
                            //}

                            if ((*it) == del) {
                                if (LOGGING_FLAG) {
                                    LogBox::push_log(">>DELETED");
                                    LogBox::push_log("GDP: " + rule.to_s());
                                    LogBox::push_log("DEL: " + del.to_s());
                                    LogBox::push_log("<<DELETED");
                                }
                                deleted_flag = true;
                                it = kb.sentenceDB.erase(it);
                            } else {
                                it++;
                            }
                        }
                    } else if (del.is_noun()) {
                        int l;
                        for (l = 0; l < sent.internal.size(); l++) {
                            if (sent.internal[l] == del.internal.front()) {
                                break;
                            }
                        }
                        if (l >= sent.internal.size()) {
                            std::cerr << "Cannot find var in sent" << std::endl;
                            throw "ERROR";
                        }

                        del.internal[0] = rule.internal[l];

                        //if (LOGGING_FLAG) {
                        //  LogBox::push_log("->>TARG: " + del.to_s());
                        //}

                        it = kb.wordDB.begin();
                        while (it != kb.wordDB.end()) {
                            //if (LOGGING_FLAG) {
                            //  LogBox::push_log("->>SEEK: " + (*it).to_s());
                            //}
                            if ((*it) == del) {
                                if (LOGGING_FLAG) {
                                    LogBox::push_log(">>DELETED");
                                    LogBox::push_log(del.to_s());
                                    LogBox::push_log("<<DELETED");
                                }
                                deleted_flag = true;
                                it = kb.wordDB.erase(it);
                            } else {
                                it++;
                            }
                        }
                    }
                }
            }
            if (deleted_flag) {
                kb.DIC_BLD = false;
                kb.word_dic.clear();
                kb.build_word_index();
            }
        }
    }

    if (DEL_LONG_RULE & LOGGING_FLAG) {
        LogBox::push_log("<<-DELETE");
    }
    Rule pseudo_meaning;
    last_selected_meaning = pseudo_meaning;

    return *this;
}

Rule
MSILMAgent::say(Rule& internal) {
    try {
        Rule res;
        if (INDEXER_FLAG) {
            if (UTTER_MINIMUM) {
                res = kb.fabricate_idx_min(internal);
            } else {
                res = kb.fabricate_idx(internal);
            }
        } else {
            if (UTTER_MINIMUM) {
                res = kb.fabricate_min_len(internal);
            } else {
                //                std::cout << "SAY!" << std::endl;
                res = kb.fabricate(internal);
                //                std::cout << "SAY!2" << std::endl;
            }
        }
        return res;


    } catch (...) {
        LogBox::refresh_log();
        throw;
    }
}

Rule
MSILMAgent::dither_say(std::vector<Rule>& internals) {
    try {
        //        std::cerr << internals.size() << std::endl;
        //         std::cerr << " 2: " << internals.size();
        std::vector<Rule> return_internals = think_meaning(internals);
        //return kb.fabricate(internal);
        //return kb.fabricate2(internal);
        Rule internal;
        if (return_internals.size() != 0) {
            internal = return_internals[0];
            last_selected_meaning.internal = internal.internal;
        } else {
            //Exception
        }
        if (INDEXER_FLAG) {
            if (UTTER_MINIMUM) {
                return kb.fabricate_idx_min(internal);
            } else {
                return kb.fabricate_idx(internal);
            }
        } else {
            if (UTTER_MINIMUM)
                return kb.fabricate_min_len(internal);
            else {
                //                std::cerr<< internal.to_s() <<std::endl;
                return kb.fabricate(internal);
            }
        }
    } catch (...) {
        LogBox::refresh_log();
        throw;
    }
}

void
MSILMAgent::hear(std::vector<Rule>& terms, std::vector<Rule> all_meanings) {
    if ((SYM_FLAG || MUT_FLAG) && EXC_FLAG) {
        std::vector<Rule>::iterator mean_it, kb1_pat_it, kb1_all_it;
        std::vector<Rule> kb1_pat, kb1_all, match_rules, unmatch_rules;
        std::vector<KnowledgeBase::PatternType> pat_patterns, all_patterns, match_patterns, unmatch_patterns;
        kb.DIC_BLD = false;
        kb.word_dic.clear();
        kb.build_word_index();

        if (INDEXER_FLAG)
            kb.indexer(all_meanings);

        mean_it = all_meanings.begin();

        for (; mean_it != all_meanings.end(); mean_it++) {
            pat_patterns.clear();
            kb1_pat = kb.grounded_rules2((*mean_it), pat_patterns);
            for (int i = 0; i < kb1_pat.size(); i++) {
                kb1_all.push_back(kb1_pat[i]);
                all_patterns.push_back(pat_patterns[i]);
            }
        }

        for (int i = 0; i < kb1_all.size(); i++) {
            if (kb1_all[i].external == terms[0].external) {
                match_rules.push_back(kb1_all[i]);
                match_patterns.push_back(all_patterns[i]);
            } else {
                unmatch_rules.push_back(kb1_all[i]);
                unmatch_patterns.push_back(all_patterns[i]);
            }
        }

        Random r;

        if (SYM_FLAG) {
            //            std::random_shuffle(match_rules.begin(), match_rules.end(), r);
            symmetry_exception_check(terms[0], match_rules, match_patterns);
        }
        if (MUT_FLAG) {
            //            std::random_shuffle(unmatch_rules.begin(), unmatch_rules.end(), r);
            mutual_exclusivity_exception_check(terms[0], unmatch_rules, unmatch_patterns);
        }
    }

    //短期記憶制限処理
    if (SHORT_MEM_SIZE != 0) {
        for (int i = 0; i < terms.size(); i++) {
            while (terms[i].external.size() > SHORT_MEM_SIZE) {
                terms[i].external.pop_back();
            }
        }
    }

    kb.send_box(terms);
}

std::vector<Rule>
MSILMAgent::think_meaning(std::vector<Rule>& internals) {
    //    std::cerr << internals.size() << std::endl;
    if (LOGGING_FLAG) {
        LogBox::push_log("START TO GUESS\n");
    }

    // std::cerr << " " << internals.size();

    std::vector<Rule> return_rule = random_think_meaning(internals);


    //

    //    std::cerr << "error" << std::endl;

    if (return_rule.size() == 1) {
        //                std::cerr << "error" << std::endl;
        if (LOGGING_FLAG) {
            LogBox::push_log("SELECTED MEANING : " + tr_vector_Rule_to_string(return_rule));
        }

    } else {
        //Exception
        throw std::exception();
    }


    return return_rule;
}

void
MSILMAgent::sym_on(void) {
    SYM_FLAG = true;
    //  KnowledgeBase::logging_on();
}

void
MSILMAgent::mut_on(void) {
    MUT_FLAG = true;
    //  KnowledgeBase::logging_on();
}

void
MSILMAgent::exc_on(void) {
    EXC_FLAG = true;
    //  KnowledgeBase::logging_on();
}

void
MSILMAgent::omission_on(void) {
    OMISSION_FLAG = true;
    //  KnowledgeBase::logging_on();
}

void
MSILMAgent::omission_off(void) {
    OMISSION_FLAG = false;
    //  KnowledgeBase::logging_on();
}

std::vector<Rule>
MSILMAgent::random_think_meaning(std::vector<Rule>& internals) {
    //        std::cerr << "error" << std::endl;

    std::vector<Rule> result;

    if (LOGGING_FLAG) {
        LogBox::push_log("USING RAMDOM THINK");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(internals));
    }
    if (internals.size() > 0) {
        int use_index = MT19937::irand() % internals.size();
        result.push_back(internals[use_index]);
        if (LOGGING_FLAG) {
            LogBox::push_log("RANDOM THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(result));
        }
        return result;
    } else {
        //            std::cerr << "error" << std::endl;
        if (LOGGING_FLAG) {
            //            std::cerr << "error" << std::endl;
            LogBox::push_log("RANDOM THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(result));
        }
        //        std::cerr << "PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(internals) << std::endl;
        return result;
    }
}

void
MSILMAgent::dither_hear(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& all_meanings) {
    bool filled_flag = false;
    std::vector<std::vector<Rule> > internals = think_meaning(filled_flag, terms, meaningss, all_meanings);
    //Rule internal;
    if (internals.size() != 0) {
        //internal = internals[0];
    } else {
        //Exception
        std::cerr << "Result of thinking is multiple meanings. \n It is impossible." << std::endl;
        throw new std::exception;
    }
    if (!filled_flag) {
        for (int i = 0; i < terms.size(); i++) {
            if (internals[i].size() == 1) {
                terms[i].internal = internals[i][0].internal;
            } else {
                std::cerr << "Thinking Result is multiple meanings. \n It is impossible." << std::endl;
                throw new std::exception;
            }
        }
    }
    last_selected_meaning.internal = (*(terms.end() - 1)).internal;

    if ((SYM_FLAG || MUT_FLAG) && EXC_FLAG) {
        std::vector<Rule>::iterator mean_it, kb1_pat_it, kb1_all_it;
        std::vector<Rule> kb1_pat, kb1_all, match_rules, unmatch_rules;
        std::vector<KnowledgeBase::PatternType> pat_patterns, all_patterns, match_patterns, unmatch_patterns;
        kb.DIC_BLD = false;
        kb.word_dic.clear();
        kb.build_word_index();

        if (INDEXER_FLAG)
            kb.indexer(all_meanings);

        mean_it = all_meanings.begin();

        for (; mean_it != all_meanings.end(); mean_it++) {
            pat_patterns.clear();
            kb1_pat = kb.grounded_rules2((*mean_it), pat_patterns);
            for (int i = 0; i < kb1_pat.size(); i++) {
                kb1_all.push_back(kb1_pat[i]);
                all_patterns.push_back(pat_patterns[i]);
            }
        }

        for (int i = 0; i < kb1_all.size(); i++) {
            if (kb1_all[i].external == terms[0].external) {
                match_rules.push_back(kb1_all[i]);
                match_patterns.push_back(all_patterns[i]);
            } else {
                unmatch_rules.push_back(kb1_all[i]);
                unmatch_patterns.push_back(all_patterns[i]);
            }
        }

        //        Random r;

        if (SYM_FLAG) {
            //            std::random_shuffle(match_rules.begin(), match_rules.end(), r);
            symmetry_exception_check(terms[0], match_rules, match_patterns);
        }
        if (MUT_FLAG) {
            //            std::random_shuffle(unmatch_rules.begin(), unmatch_rules.end(), r);
            mutual_exclusivity_exception_check(terms[0], unmatch_rules, unmatch_patterns);
        }
    }

    if (LOGGING_FLAG) {
        LogBox::push_log("CHILD AQUIRED : " + tr_vector_Rule_to_string(terms));
    }

    if (SHORT_MEM_SIZE != 0) {
        for (int i = 0; i < terms.size(); i++) {
            while (terms[i].external.size() > SHORT_MEM_SIZE) {
                terms[i].external.pop_back();
            }
        }
    }

    kb.send_box(terms);
}

std::vector<std::vector<Rule> >
MSILMAgent::think_meaning(bool& filled_flag, std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& all_meanings) {
    //    std::cerr<< "CCCCCCCCC" <<std::endl;
    //termsはexternalだけを使う
    std::vector<std::vector<Rule> > return_rules;

    std::vector<Rule>::iterator mean_it;
    std::vector<Rule> kb1_pat, kb1_all;
    std::vector<KnowledgeBase::PatternType> pat_patterns, all_patterns;
    std::vector<std::vector<Rule> >::iterator meanings_it;

    if (INDEXER_FLAG)
        kb.indexer(all_meanings);

    if (LOGGING_FLAG) {
        LogBox::push_log("START TO GUESS\n");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(meaningss) + "\n");
    }

    return_rules = meaningss;

    int max_meanings = 0;
    meanings_it = meaningss.begin();
    for (; meanings_it != meaningss.end(); meanings_it++) {
        if (max_meanings < (*meanings_it).size())
            max_meanings = (*meanings_it).size();
    }
    //    std::cerr << "AAAA" << std::endl;
    //trueなら完全一致、falseなら非完全一致
    if (true) {
        if ((SYM_FLAG || MUT_FLAG) && max_meanings > 1) {
            kb.DIC_BLD = false;
            kb.word_dic.clear();
            kb.build_word_index();

            mean_it = all_meanings.begin();
            for (; mean_it != all_meanings.end(); mean_it++) {
                pat_patterns.clear();
                kb1_pat = kb.grounded_rules2((*mean_it), pat_patterns);
                for (int i = 0; i < kb1_pat.size(); i++) {
                    kb1_all.push_back(kb1_pat[i]);
                    all_patterns.push_back(pat_patterns[i]);
                }
            }
            /*
                    for (int i = 0; i < kb1_all.size(); i++) {
                        if (kb1_all[i].external == term.external) {
                            match_rules.push_back(kb1_all[i]);
                            match_patterns.push_back(all_patterns[i]);
                        } else {
                            unmatch_rules.push_back(kb1_all[i]);
                            unmatch_patterns.push_back(all_patterns[i]);
                        }
                    }
             */
            //Random r;

            if (SYM_FLAG) {
                //std::random_shuffle(match_rules.begin(), match_rules.end(), r);
                //            internals = return_rule;
                symmetry_bias_think(terms, meaningss, kb1_all, all_patterns, return_rules);
            }
            if (MUT_FLAG) {
                //std::random_shuffle(unmatch_rules.begin(), unmatch_rules.end(), r);
                //            internals = return_rule;
                mutual_exclusivity_bias_think(terms, meaningss, kb1_all, all_patterns, return_rules);
            }
        }
    } else {
        if ((SYM_FLAG || MUT_FLAG) && max_meanings > 1) {
            //Window:mC2:2
            std::vector<std::vector<std::vector<Rule> > > meaning_pair_orders;
            std::vector<std::vector<double> > meaning_distancess;
            //Window:2
            std::vector<std::vector<Rule> > term_pairs;

            kb.DIC_BLD = false;
            kb.word_dic.clear();
            kb.build_word_index();

            mean_it = all_meanings.begin();
            for (; mean_it != all_meanings.end(); mean_it++) {
                pat_patterns.clear();
                kb1_pat = kb.grounded_rules2((*mean_it), pat_patterns);
                for (int i = 0; i < kb1_pat.size(); i++) {
                    kb1_all.push_back(kb1_pat[i]);
                    all_patterns.push_back(pat_patterns[i]);
                }
            }
            if (SYM_FLAG) {
                //                std::cerr << "CCCC" << std::endl;
                filled_flag = true;
                //意味に選択順序を与える（using hamming and levenshtein distance）
                symmetry_bias_think(terms, meaningss, kb1_all, all_patterns, term_pairs, meaning_pair_orders, meaning_distancess);
                if (MUT_FLAG) {
                    //                    std::cerr << "DDDD" << std::endl;
                    //意味の選択順序を変える（完全一致）
                    mutual_exclusivity_bias_think(terms, meaningss, kb1_all, all_patterns, term_pairs, meaning_pair_orders, meaning_distancess);
                }
                if (SYM_FLAG || MUT_FLAG) {
                    //                    std::cerr << tr_vector_Rule_to_string(terms); 
                    //選択順序が最も高い意味をreturn_rulesにいれる
                    decide_likelihood(terms, term_pairs, meaning_pair_orders);

                }
            }

        }
    }

    //    std::cerr << "BBBB" << std::endl;

    max_meanings = 0;
    meanings_it = meaningss.begin();
    for (; meanings_it != meaningss.end(); meanings_it++) {
        if (max_meanings < (*meanings_it).size())
            max_meanings = (*meanings_it).size();
    }

    if (max_meanings > 1 && (!filled_flag)) {
        for (int i = 0; i < return_rules.size(); i++) {
            if (return_rules[i].size() > 1)
                return_rules[i] = random_think_meaning(return_rules[i]);
        }
    }

    if (LOGGING_FLAG) {
        LogBox::push_log("SELECTED MEANING : " + tr_vector_Rule_to_string(return_rules));
    }

    //    std::cerr<< "DDDDDDDDD" <<std::endl;
    return return_rules;
}

void
MSILMAgent::symmetry_bias_think(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns, std::vector<std::vector<Rule> >& return_rules) {
    //    std::cerr<< "AAAAAAA" <<std::endl;
    //候補がない場合、NULLを返す
    if (LOGGING_FLAG) {
        LogBox::push_log("USING SYMMETRY BIAS THINK");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(meaningss));
    }
    //std::vector<std::vector<Rule> > result;
    std::vector<Rule>::iterator meanings_it1, meanings_it2, refer_it;
    std::vector<std::vector<Rule> >::iterator meaningss_it;
    std::vector<Rule> meaning_stack;

    //window内のbias
    for (int i = 0; i < terms.size(); i++) {
        for (int j = i + 1; j < terms.size(); j++) {
            if (terms[i].external == terms[j].external) {
                meaning_stack.clear();
                //meaningss[i],meaningss[j]
                meanings_it1 = meaningss[i].begin();
                for (; meanings_it1 != meaningss[i].end(); meanings_it1++) {
                    meanings_it2 = meaningss[j].begin();
                    for (; meanings_it2 != meaningss[j].end(); meanings_it2++) {
                        if ((*meanings_it1).internal == (*meanings_it2).internal)
                            meaning_stack.push_back(*meanings_it1);
                    }
                }

                if (meaning_stack.size() != 0) {
                    meaningss[i] = meaning_stack;
                    meaningss[j] = meaning_stack;
                }

            }
        }
    }

    //言語知識からのbias
    int ref_count=0;
    refer_it = reference.begin();
    for (; refer_it != reference.end(); refer_it++) {
        for (int i = 0; i < terms.size(); i++) {
            Rule temp = reference[0];
            bool exist_f = false;
            if ((*refer_it).external == terms[i].external && meaningss[i].size() != 1) {
                bool find = false;
                if (!exist_f) {
                    meanings_it1 = meaningss[i].begin();
                    for (; meanings_it1 != meaningss[i].end(); meanings_it1++) {
                        if ((*meanings_it1).internal == (*refer_it).internal)
                            find = true;
                    }
                    if (find) {
                        meaningss[i].clear();
                        meaningss[i].resize(1);
                        meaningss[i][0] = reference[0]; //文ならなんでもいい
                        meaningss[i][0].internal = (*refer_it).internal;
                        meaningss[i][0].external.clear();
                        temp.internal = meaningss[i][0].internal;
                        exist_f = true;
                    }
                }
                if(EXC_FLAG && exist_f&&(!find) && temp.internal != (*refer_it).internal){//wide
                    kb.prohibited(patterns[ref_count]);
                }
            }
        }
        ref_count++;
    }

    return_rules = meaningss;

    if (LOGGING_FLAG) {
        LogBox::push_log("SYMMETRY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(return_rules));
    }

    /*bool flag = false;
    bool find = false;
    bool in;

    //    std::cerr << "START SYM" <<std::endl;

    for (int i = 0; i < reference.size(); i++) {
        //        std::cerr << (*ref_it).to_s() << std::endl;
        in = false;
        find = false;

        internals_it = internals.begin();
        for (; internals_it != internals.end(); internals_it++) {
            find = (find || (reference[i].internal == (*internals_it).internal));

            if ((!flag)&&(reference[i].internal == (*internals_it).internal)) {
                in = true;
                result.push_back(*internals_it);
            }

        }
        //        std::cerr << flag << std::endl;
        //選ばれなかったら間違いとする（広範囲的）
        if (flag && EXC_FLAG) {
            //            std::cerr << "prohibit" << std::endl;
            kb.prohibited(patterns[i]);
        }
        //見つからなかったら間違いとする（狭範囲的）
        //        if ((!find) && EXC_FLAG) {
        //            kb.prohibited(patterns[i]);
        //        }

        if ((!flag) && in)
            flag = true;
    }

    if (LOGGING_FLAG) {
        LogBox::push_log("SYMMETRY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(result));
    }
    return_rule = flag ? result : internals;*/
    //    std::cerr<< "BBBBBBBBBB" <<std::endl;
}

void
MSILMAgent::mutual_exclusivity_bias_think(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns, std::vector<std::vector<Rule> >& return_rules) {
    //候補がない場合、NULLを返す
    //exception未実装
    if (LOGGING_FLAG) {
        LogBox::push_log("USING MUTUAL EXCLUSIVITY BIAS THINK");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(meaningss));
    }

    //std::vector<std::vector<Rule> > result;
    std::vector<Rule>::iterator meanings_it1, meanings_it2, refer_it;
    std::vector<std::vector<Rule> >::iterator meaningss_it;
    //std::vector<Rule> meaning_stack;
    bool m_fl = false;

    //window内のbias
    for (int i = 0; i < terms.size(); i++) {
        for (int j = i+1; j < terms.size(); j++) {
            if (i != j && terms[i].external != terms[j].external && meaningss[i].size() == 1 && meaningss[j].size() > 1) {
                //ayashii
                //meaning_stack.clear();
                //meaningss[i],meaningss[j]
                //meanings_it1=meaningss[i].begin();
                //for(;meanings_it1!=meaningss[i].end();meanings_it1++){
                meanings_it2 = meaningss[j].begin();
                for (; meanings_it2 != meaningss[j].end(); meanings_it2++) {
                    if (meaningss[i][0].internal == (*meanings_it2).internal) {
                        meaningss[j].erase(meanings_it2);
                        break;
                    }
                }
                //}
            }
        }
    }

    //言語知識からのbias
    refer_it = reference.begin();
    for (; refer_it != reference.end(); refer_it++) {
        for (int i = 0; i < terms.size(); i++) {
            if ((*refer_it).external != terms[i].external && meaningss[i].size() > 1) {
                meanings_it2 = meaningss[i].begin();
                for (; meanings_it2 != meaningss[i].end(); meanings_it2++) {
                    if ((*refer_it).internal == (*meanings_it2).internal) {
                        meaningss[i].erase(meanings_it2);
                        break;
                    }
                }
            }
        }
    }

    while (true) {
        m_fl = true;
        //window内のbias
        for (int i = 0; i < terms.size(); i++) {
            for (int j = i+1; j < terms.size(); j++) {
                if (i != j && terms[i].external != terms[j].external && meaningss[i].size() == 1 && meaningss[j].size() > 1) {
                    meanings_it2 = meaningss[j].begin();
                    for (; meanings_it2 != meaningss[j].end(); meanings_it2++) {
                        if (meaningss[i][0].internal == (*meanings_it2).internal) {
                            meaningss[j].erase(meanings_it2);
                            m_fl = false;
                            break;
                        }
                    }
                }
            }
        }
        if (m_fl)
            break;
    }

    return_rules = meaningss;

    if (LOGGING_FLAG) {
        LogBox::push_log("MUTUAL EXCLUSIVITY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(return_rules));
    }


    /*std::vector<Rule> result;
    std::vector<Rule> internals_copy = internals;
    std::vector<Rule>::iterator ref_it, copy_it;

    ref_it = reference.begin();
    for (; (ref_it != reference.end() && internals_copy.size() != 0); ref_it++) {
        copy_it = internals_copy.begin();
        for (; copy_it != internals_copy.end(); copy_it++) {
            if ((*ref_it).internal == (*copy_it).internal) {
                internals_copy.erase(copy_it);
                break;
            }
        }
    }

    if (EXC_FLAG) {
        if (internals_copy.size() == 0) {
            result = random_think_meaning(internals);
            //exception
            if (result.size() != 1)
                throw new std::exception;

            for (int i = 0; i < reference.size(); i++) {
                if (reference[i].internal == result[0].internal) {
                    kb.prohibited(patterns[i]);
                }
            }
        } else {
            result = internals_copy;
        }
    } else {
        if (internals_copy.size() == 0) {
            result = random_think_meaning(internals);
        } else {
            result = internals_copy;
        }
    }

    if (LOGGING_FLAG) {
        LogBox::push_log("MUTUAL EXCLUSIVITY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(result));
    }

    return_rule = result;*/
}

void
MSILMAgent::symmetry_bias_think(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns, std::vector<std::vector<Rule> >& term_pairs, std::vector<std::vector<std::vector<Rule> > >& meaning_pair_orders, std::vector<std::vector<double> >& meaning_distancess) {
    //候補がない場合、NULLを返す
    if (LOGGING_FLAG) {
        LogBox::push_log("USING SYMMETRY BIAS THINK");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(meaningss));
    }

    //    std::cerr << "EEEE" << std::endl;

    bool window_flag;
    int index, index2;
    std::vector<int> index_buf;
    std::vector<int>::iterator index_buf_it;
    std::vector<Rule> meanings, temp;
    std::vector<Rule>::iterator refer_it;
    double min_dist = 10, lev;
    Rule base_rule, min_dist_rule;
    term_pairs.clear();
    //    std::cerr << "\n" << terms.size() << std::endl;
    while (index_buf.size() != terms.size()) {
        //base_rule
        for (int i = 0; i < terms.size(); i++) {
            index_buf_it = std::find(index_buf.begin(), index_buf.end(), i);
            if (index_buf_it == index_buf.end()) {
                index = i;
                index_buf.push_back(index);
                break;
            }
        }
        //        std::cerr << "VVVV" << std::endl;
        //調べる終端記号列（発話）の設定
        base_rule = terms[index];
        //ひっかからなかったときの初期設定
        window_flag = false;
        meanings = meaningss[index];
        //  std::cerr << "FFFF" << std::endl;
        //temp_it=temp.begin();
        for (int i = 0; i < terms.size(); i++) {
            index_buf_it = std::find(index_buf.begin(), index_buf.end(), i);
            if (index_buf_it == index_buf.end()) {
                lev = Distance::levenstein(base_rule.external, terms[i].external);
                if (min_dist > lev) {
                    window_flag = true;
                    min_dist = lev;
                    min_dist_rule = terms[i];
                    meanings = meaningss[i];
                    index2 = i;
                }
            }
        }
        //        std::cerr << "GGGG" << std::endl;
        refer_it = reference.begin();
        for (; refer_it != reference.end(); refer_it++) {
            lev = Distance::levenstein(base_rule.external, (*refer_it).external);
            if (min_dist > lev) {
                window_flag = false;
                min_dist = lev;
                min_dist_rule = (*refer_it);
                temp.clear();
                temp.push_back(*refer_it);
                meanings = temp;
            }
        }
        //termのpairをつくる
        temp.clear();
        temp.push_back(base_rule);
        if (window_flag) {
            temp.push_back(min_dist_rule);
            index_buf.push_back(index2);
        }
        std::vector<Rule> empty;
        term_pairs.push_back(empty);
        term_pairs[term_pairs.size() - 1] = temp;
        //base_rule.to_s();
        //std::cerr << "HHHH" << std::endl;
        //meaning_pairを作る
        //meaningss[index]とmeaningsで行う．
        //meaning_pairs.push_back(maening_pair)
        //meaning_pair_orders.push_back(meaning_pairs)
        std::vector<std::vector<Rule> > meaning_pairs, meaning_pairs_buf;
        meaning_pair_orders.push_back(meaning_pairs);
        //meaning_pairs_bufに入っている2つの意味のハミング距離を計算
        std::vector<double> distances, distances_buf;
        meaning_distancess.push_back(distances);
        //        std::cerr << "NNNN" << std::endl;
        //        std::cerr << terms.size() << ":" << meaningss.size() << std::endl;
        for (int i = 0; i < meaningss[index].size(); i++) {
            //            std::cerr << "MMMM" << std::endl;
            for (int j = 0; j < meanings.size(); j++) {
                //                std::cerr << "JJJJ" << std::endl;
                std::vector<Rule> meaning_pair;
                meaning_pairs_buf.push_back(meaning_pair);
                temp.clear();
                temp.push_back(meaningss[index][i]);
                //                std::cerr << "KKKK" << std::endl;
                temp.push_back(meanings[j]);
                //                std::cerr << "LLLL" << std::endl;
                meaning_pairs_buf[meaning_pairs_buf.size() - 1] = temp;
                distances_buf.push_back(Distance::hamming(meaningss[index][i].internal, meanings[j].internal));
            }
        }
        //        std::cerr << "IIII" << std::endl;
        //        std::cerr << distances_buf.size() << ":" << meaning_pairs_buf.size() << std::endl;
        //並び替え
        double ham_dist; //交換用
        std::vector<Rule> tmp_rules; //交換用
        for (int i = 0; i < (meaning_pairs_buf.size() - 1); i++) {
            //            std::cerr << "PPPP" << std::endl;
            for (int j = 0; j < (meaning_pairs_buf.size() - i - 1); j++) {
                //                std::cerr << "QQQQ" << std::endl;
                //                std::cerr << i << ":" << j << std::endl;
                ham_dist = distances_buf[j];
                //                std::cerr << "SSSS" << std::endl;
                if (ham_dist > distances_buf[j + 1]) {
                    //exchanging distance
                    //                    std::cerr << "TTTT" << std::endl;
                    distances_buf[j] = distances_buf[j + 1];
                    distances_buf[j + 1] = ham_dist;
                    //exchanging meaning
                    //                    std::cerr << "UUUU" << std::endl;
                    tmp_rules = meaning_pairs_buf[j];
                    meaning_pairs_buf[j] = meaning_pairs_buf[j + 1];
                    meaning_pairs_buf[j + 1] = tmp_rules;
                }
                //                std::cerr << "RRRR" << std::endl;
            }
        }
        // std::cerr << "OOOO" << std::endl;

        meaning_distancess[meaning_distancess.size() - 1] = distances_buf;
        meaning_pair_orders[meaning_pair_orders.size() - 1] = meaning_pairs_buf;

    }
    //    std::cerr << term_pairs.size() << std::endl;

    if (LOGGING_FLAG) {
        LogBox::push_log("SYMMETRY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(meaning_pair_orders[0]));
    }
}

void
MSILMAgent::mutual_exclusivity_bias_think(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& meaningss, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns, std::vector<std::vector<Rule> >& term_pairs, std::vector<std::vector<std::vector<Rule> > >& meaning_pair_orders, std::vector<std::vector<double> >& meaning_distancess) {
    //候補がない場合、NULLを返す
    if (LOGGING_FLAG) {
        LogBox::push_log("USING MUTUAL EXCLUSIVITY BIAS THINK");
        LogBox::push_log("PROSPECTIVE MEANINGS:\n" + tr_vector_Rule_to_string(meaningss));
    }
    //std::cerr << "WWWW" << std::endl;
    std::vector<Rule>::iterator refer_it;
    std::vector<std::vector<Rule> >::iterator orders_it;
    //std::cerr << tr_vector_Rule_to_string(term_pairs) << std::endl;
    for (int i = 0; i < term_pairs.size(); i++) {
        //term_pairs[i]
        //インデックスi番目のmeaning_pair_ordersをかえる
        refer_it = reference.begin();
        for (; refer_it < reference.end(); refer_it++) {
            if ((*refer_it).external != term_pairs[i][0].external) {
                //meaning_pair_orders[i][x][0]
                //(*refer_it).internalと同じ意味をmeaning_pair_orders[i]から消す
                bool check_fl = false;
                while (!check_fl) {
                    check_fl = true;
                    orders_it = meaning_pair_orders[i].begin();
                    for (; orders_it != meaning_pair_orders[i].end(); orders_it++) {
                        if ((*orders_it)[0].internal == (*refer_it).internal && meaning_pair_orders[i].size() != 1) {
                            meaning_pair_orders[i].erase(orders_it);
                            check_fl = false;
                            break;
                        }
                    }
                }
            }
            if (term_pairs[i].size() == 2) {
                if ((*refer_it).external != term_pairs[i][1].external) {
                    //meaning_pair_orders[i][x][1]
                    //(*refer_it).internalと同じ意味をmeaning_pair_orders[i]から消す
                    bool check_fl = false;
                    while (!check_fl) {
                        check_fl = true;
                        orders_it = meaning_pair_orders[i].begin();
                        for (; orders_it != meaning_pair_orders[i].end(); orders_it++) {
                            if ((*orders_it)[1].internal == (*refer_it).internal && meaning_pair_orders[i].size() != 1) {
                                meaning_pair_orders[i].erase(orders_it);
                                check_fl = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

    }

    if (LOGGING_FLAG) {
        LogBox::push_log("MUTUAL EXCLUSIVITY BIAS THINK\nRETURN MEANINGS:\n" + tr_vector_Rule_to_string(meaning_pair_orders[0]));
    }
    //    std::cerr << "XXXX" << std::endl;

}

void MSILMAgent::decide_likelihood(std::vector<Rule>& terms, std::vector<std::vector<Rule> >& term_pairs, std::vector<std::vector<std::vector<Rule> > >& meaning_pair_orders) {

    //    std::cerr << "YYYY" << std::endl;
    if (LOGGING_FLAG) {
        LogBox::push_log("DECIDE MAXIMUM LIKELIHOOD MEANNING\nUTTERANCES:\n" + tr_vector_Rule_to_string(term_pairs));
    }

    std::vector<Rule>::iterator it = terms.begin();
    int i = 0;
    for (; i < term_pairs.size(); i++) {

        if (LOGGING_FLAG) {
            LogBox::push_log("\nTARGET TERMINAL STRING:\n" + tr_vector_Rule_to_string(term_pairs[i]) + "\nPROSPECTIVE MEANING PAIR:\n" + tr_vector_Rule_to_string(meaning_pair_orders[i]));
        }

        (*it).external = term_pairs[i][0].external;
        (*it).internal = meaning_pair_orders[i][0][0].internal;
        it++;
        if (term_pairs[i].size() == 2) {
            (*it).external = term_pairs[i][1].external;
            (*it).internal = meaning_pair_orders[i][0][1].internal;
            it++;
        }
    }
    //    std::cerr << "ZZZZ" << std::endl;
}

void MSILMAgent::symmetry_exception_check(Rule term, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns) {

    for (int i = 0; i < reference.size(); i++) {

        if ((term.external == reference[i].external && term.internal != reference[i].internal)) {
            kb.prohibited(patterns[i]);
        }

    }
}

void MSILMAgent::mutual_exclusivity_exception_check(Rule term, std::vector<Rule>& reference, std::vector<KnowledgeBase::PatternType>& patterns) {
    for (int i = 0; i < reference.size(); i++) {

        if ((term.internal == reference[i].internal && term.external != reference[i].external)) {
            kb.prohibited(patterns[i]);
        }

    }
}

Rule MSILMAgent::return_last_selected_meaning() {
    return last_selected_meaning;
}

//waste

std::string
MSILMAgent::tr_vector_Rule_to_string(std::vector<Rule> vector) {
    if (vector.size() != 0) {
        std::string res = "(";
        std::vector<Rule>::iterator rule_it = vector.begin();

        res = res + (*rule_it).to_s();
        rule_it++;
        for (; rule_it != vector.end(); rule_it++) {
            res = res + "," + (*rule_it).to_s();
        }
        return (res + ")");
    } else {
        return "no rules";
    }
}

std::string
MSILMAgent::tr_vector_Rule_to_string(std::vector<std::vector<Rule> > vector) {
    if (vector.size() != 0) {
        std::string res = "(";
        std::vector<std::vector<Rule> >::iterator rule_it = vector.begin();

        res = res + tr_vector_Rule_to_string(*rule_it);
        rule_it++;
        for (; rule_it != vector.end(); rule_it++) {
            res = res + "," + tr_vector_Rule_to_string(*rule_it);
        }
        return (res + ")");
    } else {
        return "no rules";
    }
}