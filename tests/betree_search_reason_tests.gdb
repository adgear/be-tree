set pagination off
set logging file gdb.output
set logging enabled on

break free_report_err
    command 1
    if hashtable_get(report->reason_sub_id_list, "b") 
        print "b"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "b"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "b")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "i")
        print "i"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "i"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "i")))->body[$i]
            set $i = $i + 1
        end
    end 
    if hashtable_get(report->reason_sub_id_list, "f")
        print "f"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "f"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "f")))->body[$i]
            set $i = $i + 1
        end
    end 
    if hashtable_get(report->reason_sub_id_list, "s")
        print "s"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "s"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "s")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "il")
        print "il"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "il"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "il")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "sl")
        print "sl"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "sl"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "sl")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "seg")
        print "seg"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "seg"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "seg")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "segments_with_timestamp")
        print "segments_with_timestamp"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "segments_with_timestamp"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "segments_with_timestamp")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "geo")
        print "geo"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "geo"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "geo")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "frequency_caps")
        print "frequency_caps"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "frequency_caps"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "frequency_caps")))->body[$i]
            set $i = $i + 1
        end
    end
    if hashtable_get(report->reason_sub_id_list, "now")
        print "now"
        set $size = ((hashtable*)hashtable_get(report->reason_sub_id_list, "now"))->size
        set $i = 0
        while $i < $size
            print ((arraylist*)((hashtable*)hashtable_get(report->reason_sub_id_list, "now")))->body[$i]
            set $i = $i + 1
        end
    end
    print ""
    print "DUMP HASHTABLE"
    set $size = report->reason_sub_id_list->capacity
    set $i = 0
    while $i < $size
        if report->reason_sub_id_list->body[$i].key 
            print report->reason_sub_id_list->body[$i].key 
            if report->reason_sub_id_list->body[$i].value
                set $size2 = ((arraylist*)report->reason_sub_id_list->body[$i].value)->size
                set $i2 = 0
                while $i2 < $size2
                    print ((arraylist*)report->reason_sub_id_list->body[$i].value)->body[$i2]
                    set $i2 = $i2 + 1
                end
            end
        end
        set $i = $i + 1
    end
    print ""
    continue
end

run

set logging enabled off
quit
