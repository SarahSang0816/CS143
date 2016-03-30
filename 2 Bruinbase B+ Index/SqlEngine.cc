/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <fstream>
#include "SqlEngine.h"

// external functions and variables for load file and sql command parsing
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
    fprintf(stdout, "Bruinbase> ");
    
    // set the command line input and start parsing user input
    sqlin = commandline;
    sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
    // SqlParser.y by bison (bison is GNU equivalent of yacc)
    
    return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
    RecordFile rf;   // RecordFile containing the table
    RecordId   rid;  // record cursor for table scanning
    BTreeIndex tblidx;
    vector<SelCond> valcond;
    
    RC     rc;
    int    key;
    string value;
    int    count;
    
    // open the table file
    if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
        fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
        return rc;
    }
    
    
    // check if index exists
    if ((rc = tblidx.open(table + ".idx", 'r')) == 0)
    {
        count = 0;
        int keyMin = INT_MIN, keyMax = INT_MAX;
        bool minequal = false, maxequal = false;
        // check inequality
        bool keyconstraint = false;
        for (unsigned i = 0; i < cond.size(); i++)
            if (cond[i].attr == 1 && cond[i].comp != SelCond::NE)
                keyconstraint = true;
        vector<int> nelist;
        for (unsigned i = 0; i < cond.size(); i++)
        {
            if (cond[i].attr == 2) valcond.push_back(cond[i]);
            else
            {
                int val = atoi(cond[i].value);
                switch (cond[i].comp) {
                    case SelCond::EQ:
                        if (val>keyMin)
                        {
                            keyMin = val;
                            minequal = true;
                        }
                        if (val<keyMax)
                        {
                            keyMax = val;
                            maxequal = true;
                        }
                        break;
                    case SelCond::NE:
                        nelist.push_back(val);
                        break;
                    case SelCond::GT:
                        if (val>keyMin || (val==keyMin && minequal))
                        {
                            keyMin = val;
                            minequal = false;
                        }
                        break;
                    case SelCond::LT:
                        if (val<keyMax || (val==keyMax && maxequal))
                        {
                            keyMax = val;
                            maxequal = false;
                        }
                        break;
                    case SelCond::GE:
                        if (val>keyMin)
                        {
                            keyMin = val;
                            minequal = true;
                        }
                        break;
                    case SelCond::LE:
                        if (val<keyMax)
                        {
                            keyMax = val;
                            maxequal = true;
                        }
                        break;
                }
            }
        }// keyconstraint = false when only has '!='
        if (!(keyconstraint || ((cond.size()==0) && (attr == 1 || attr == 4)))) {
            goto direct_scan;
        }
        
        if (keyMin > keyMax || (keyMax == keyMin && !(minequal && maxequal))) {
            goto exit_select;
            
        }
        
        IndexCursor startidx, endidx, temp, currentidx;
        tblidx.locate(keyMin, startidx);

        tblidx.locate(keyMax, endidx);
        temp = startidx;
        rc = tblidx.readForward(startidx, key, rid);

        if(rc < 0)
            goto exit_select;
        
        if (key > keyMin || minequal) startidx = temp;
        temp = endidx;
        
        rc = tblidx.readForward(endidx, key, rid); // endidx is the end of index
        if (rc < 0 || key > keyMax || !maxequal) endidx = temp;

        currentidx = startidx;
        while (tblidx.readForward(currentidx, key, rid) == 0)
        {
            bool ne = false;
            for (int i = 0; i<nelist.size(); i++)
                if (key == nelist[i]) ne = true;
            if (ne) continue;
            if (valcond.empty() && (attr==1||attr==4))// no condition on value
            {
                count++;
                if (attr == 1)
                    fprintf(stdout, "%d\n", key);
            }
            else
            {
                if ((rc = rf.read(rid, key, value)) < 0) {
                    fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
                    goto exit_select;
                }
                if (meetCond(valcond, key, value))// only check conditions on value
                {
                    count++;
                    // print the tuple
                    switch (attr) {
                        case 1:  // SELECT key
                            fprintf(stdout, "%d\n", key);
                            break;
                        case 2:  // SELECT value
                            fprintf(stdout, "%s\n", value.c_str());
                            break;
                        case 3:  // SELECT *
                            fprintf(stdout, "%d '%s'\n", key, value.c_str());
                            break;
                    }
                }
            }
            
            if (currentidx.pid == endidx.pid && currentidx.eid >= endidx.eid) {
                break;
            }
        }
        
        // print matching tuple count if "select count(*)"
        if (attr == 4) {
            fprintf(stdout, "%d\n", count);
        }
        goto exit_select;
    }
    
direct_scan:
    
    // scan the table file from the beginning
    rid.pid = rid.sid = 0;
    count = 0;
    while (rid < rf.endRid()) {
        // read the tuple
        if ((rc = rf.read(rid, key, value)) < 0) {
            fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
            goto exit_select;
        }
        
        // check the conditions on the tuple
        // the condition is met for the tuple.
        // increase matching tuple counter
        if (meetCond(cond, key, value))
        {
            count++;
            
            // print the tuple
            switch (attr) {
                case 1:  // SELECT key
                    fprintf(stdout, "%d\n", key);
                    break;
                case 2:  // SELECT value
                    fprintf(stdout, "%s\n", value.c_str());
                    break;
                case 3:  // SELECT *
                    fprintf(stdout, "%d '%s'\n", key, value.c_str());
                    break;
            }
        }
        
        
        // move to the next tuple
        ++rid;
    }
    // print matching tuple count if "select count(*)"
    if (attr == 4) {
        fprintf(stdout, "%d\n", count);
    }
    rc = 0;
    
    // close the table file and return
exit_select:
    rf.close();
    return rc;
}

bool SqlEngine::meetCond(const std::vector<SelCond>& cond, const int key, const string& value)
{
    int diff = 0;
    // check the conditions on the tuple
    for (unsigned i = 0; i < cond.size(); i++) {
        // compute the difference between the tuple value and the condition value
        switch (cond[i].attr) {
            case 1:
                diff = key - atoi(cond[i].value);
                break;
            case 2:
                diff = strcmp(value.c_str(), cond[i].value);
                break;
        }
        
        // skip the tuple if any condition is not met
        switch (cond[i].comp) {
            case SelCond::EQ:
                if (diff != 0) return false;
                break;
            case SelCond::NE:
                if (diff == 0) return false;
                break;
            case SelCond::GT:
                if (diff <= 0) return false;
                break;
            case SelCond::LT:
                if (diff >= 0) return false;
                break;
            case SelCond::GE:
                if (diff < 0) return false;
                break;
            case SelCond::LE:
                if (diff > 0) return false;
                break;
        }
    }
    return true;
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
    /* your code here */
    RC rc;
    ifstream infile(loadfile.c_str());
    if (!infile.is_open())
    {
        rc = -1;
        fprintf(stderr, "Error: file %s doesn't exist or cannot open\n", loadfile.c_str());
        return rc;
    }
    string line;
    int key;
    string value;
    RecordFile outfile;
    if ((rc = outfile.open(table+".tbl", 'w')) < 0)
    {
        fprintf(stderr, "Error: Cannot create the output file %s.tbl\n", table.c_str());
        return rc;
    }
    RecordId rid;
    
    BTreeIndex tblidx;
    if (index)
    {
        if ((rc = tblidx.open(table+".idx", 'w'))<0)
        {
            fprintf(stderr, "Error: Cannot create the index file %s.idx\n", table.c_str());
            return rc;
        }
    }
    int count = 0;
    while (getline(infile, line))
    {
        count++;
        parseLoadLine(line, key, value);
        outfile.append(key, value, rid);
        if (index && ((rc = tblidx.insert(key, rid))<0))
        {
            fprintf(stderr, "Error: Cannot insert the tuple with key = %d\n", key);
            return rc;
        }
        
    }
    outfile.close();
    infile.close();
    if (index && (rc = tblidx.close())<0)
    {
        fprintf(stderr, "Error: Cannot close the index file");
        return rc;
    }
    return 0;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }
    
    // get the integer key value
    key = atoi(s);
    
    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }
    
    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) {
        value.erase();
        return 0;
    }
    
    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }
    
    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }
    
    return 0;
}
