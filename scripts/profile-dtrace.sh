#!/bin/sh

dtrace -c ./sled <<EOF

unsigned long long totalTime;
self uint64_t lastEntry;

dtrace:::BEGIN
{
    totalTime = 0;
}

pid$target:sled:*px_client_executeline*:entry
{
    self->lastEntry = vtimestamp;
}

pid$target:sled:*px_client_executeline*:return
{
    totalTime = totalTime + (vtimestamp - self->lastEntry);
    /*@timeByThread[tid] = sum(vtimestamp - self->lastEntry);*/
}

dtrace:::END
{
    printf( "\n\nTotal time %dms\n" , totalTime/1000000 )
}
EOF
