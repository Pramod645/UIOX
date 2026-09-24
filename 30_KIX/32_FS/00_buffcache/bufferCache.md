| Source concept | Implementation |
| --- | --- |
| Buffer header with 8 fields | BufHdr: dev, blkno, status, data[], hash_next/prev, free_next/prev |
| Five BUF_* status conditions | BUF_LOCKED, BUF_VALID, BUF_DELWRITE, BUF_IOBUSY, BUF_WANTED, BUF_OLD, BUF_ASYNC |
| Circular doubly-linked free list with dummy head | free_head sentinel; fl_insert_before/after, fl_remove, fl_pop_head |
| Hash queues blkno mod N | hash_heads[NUM_HASH_QUEUES]; hash_slot(dev, blkno) |
| Algorithm getblk — 5 scenarios | All five in the while(1) loop with explicit scenario comments |
| Algorithm brelse — tail/head placement | Valid+not-old → tail (LRU); invalid or old → head |
| Algorithm bread | getblk + disk read if not valid |
| Algorithm breada| First block + async second block; brelse on async completion |
| Algorithm bwrite sync/async/delayed | Three paths controlled by sync/delayed parameters |
| bdwrite convenience | Calls bwrite(buf, false, true) |
| bflush for sync(2) / umount(2) | Scans pool for BUF_DELWRITE on the given device |