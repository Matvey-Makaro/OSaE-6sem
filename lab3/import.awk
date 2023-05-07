BEGIN {
    FS = ","
    OFS = ","
    IS_NEXT_COLUMNS = 0
    IS_TABLE_NEXT = 1
}

{

    if ($1 == "TABLE" && NF == 2) {
        TABLE_NAME = $2;
        # print "Table name: ", TABLE_NAME;
        IS_NEXT_COLUMNS = 1;
        IS_TABLE_NEXT = 0;
    }
    else if ($1 == "COLUMNS") {
        if (NF <= 1)
        {
            print "Error!: Number of columns cannot be 0. Line: ", NR;
            exit
        }
        else if (IS_TABLE_NEXT == 1)
        {
            print "ERROR!: TABLE expected in line " NR;
            exit
        }
        else
        {
            COLUMNS = $2;
            for (i = 3; i <= NF; i++) {
                COLUMNS = COLUMNS OFS $i
            }
            NUM_COLUMNS = NF - 1;
            IS_NEXT_COLUMNS = 0;
        }
    }
    else {
        if (IS_NEXT_COLUMNS == 1) {
            print "ERROR!: COLUMNS expected in line " NR;
            exit
        }
        if (IS_TABLE_NEXT == 1)
        {
            print "ERROR!: TABLE expected in line " NR;
            exit
        }
        if (NUM_COLUMNS != NF) {
            print "ERROR: incorrect number of columns in line " NR;
            #print "NUM COL: " NUM_COLUMNS
            #print "COLS: " COLUMNS
            exit
        }
        else
        {
            printf "INSERT INTO %s (%s) VALUES (", TABLE_NAME, COLUMNS;
        
            for (i = 1; i <= NF; i++) {
                printf "'%s'", $i;
                if (i != NF) { 
                    printf ", "; 
                }
            }
            print ");"
        }
    }
}
