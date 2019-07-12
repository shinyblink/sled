// Copyright (c) 2019, Jonathan Cyriax Brast
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

static int * data;
static int * data2;
static int n;
static int logn;

static int sorting_algorithm=12;
static int __rval=0;

// highlighting
static int h1;
static int h2;

#define SORTING_ALGORITHM_MAX_ID 17

// pseudo jump and link
static int __yield_value;
#define CONTINUE(x)         \
    if (__yield_value == x) \
        goto label##x;
#define YIELD(x)        \
    __yield_value = x;  \
    return 0;           \
    label##x:

#define swap(a,b)       \
    {int __tmp = data[a];\
    data[a] = data[b];  \
    data[b] = __tmp;}

#define cmp_swap(a,b)       \
    h1 = a; h2 = b;         \
    if (data[a] > data[b]){ \
        swap(a,b);          \
        inversions++;       \
    }

// sorting algorithm internals
static int i,j;
static int a,b,c;
static int inversions;
static int step,stage,stride;
static int iMin;
static int start,end,child,root,swapable;
static int last;
static int partner;
static int log_size;
static int l,r,ll,rr,stack_p;

static int pairwise_sorting_net(){
    CONTINUE(1);
    CONTINUE(2);
    log_size = 0;
    for (int i=1;i<n;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 0;stage<log_size;stage++){
        for (i = 0;i<n;i++){
            int box_start = (i>>(stage+1))<<(stage+1);
            int step_size = 1<<stage;
            partner = (i-step_size < box_start) ? i + step_size : i - step_size;
            if (partner > i && partner < n) {
                cmp_swap(i,partner);
                YIELD(1);

            }
        }
    }
    //printf("logsize %d\n",log_size);
    for (stage=log_size-2;stage>=0;stage--){
        //printf("====(%d)====\n",stage);
        for (step=log_size-1-stage;step>=1;step--){
            //printf("--=(%d:%d)=--\n",stage,step);
            for (i = 1<<stage;i+stride<n;i+=(2<<stage)){
                stride = ((1<<step)-1)<<stage;
                //printf("-(%d:%d) @%d [%d]-\n",stage,step,i,stride);
                for (j=0;j<(1<<stage);j++){
                    if (i+j+stride >= n) continue;
                    cmp_swap(i+j,i+j+stride);
                    //printf(" %d <-> %d (%d:%d)\n",i+j,i+j+stride);
                    YIELD(2);
                }
            }
        }
    }
    return 1;
}
static int bitonic_sort(){
    CONTINUE(1);
    log_size = 0;
    for (int i=1;i<n;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 1;stage<log_size+1;stage++){
        for (step=1;step<=stage;step++){
            for (i = 0;i<n;i++){
                int box_size = 1<<(stage-step+1);
                int box_start = (i>>(stage-step+1))<<(stage-step+1);
                if (step == 1) partner = box_start + box_size - (i%box_size)-1;
                else {
                    partner = (i-box_size/2 < box_start) ? i + box_size/2 : i - box_size/2;
                }
                //printf("    %d <-> %d b:%d-%d (%d)\n",i,partner,box_start,box_start+box_size,box_size);

                if (partner > i && partner < n) {
                    cmp_swap(i,partner);
                    YIELD(1);

                }
            }
        }
    }
    return 1;
}

static int odd_even_mergesort(){
    CONTINUE(1);

    log_size = 0;
    for (int i=1;i<n;i*=2)log_size++;
    //printf("ls:%d\n",log_size);
    for (stage = 1;stage<log_size+1;stage++){
        for (step=1;step<=stage;step++){
            //printf("%d:%d %d\n",stage,step,1<<(stage-step));
            for (i = 0;i<n;i++){
                if (step == 1) partner = i ^ (1 << (stage-1));
                else {
                    int stride = 1 << (stage-step);
                    int box = (i >> stage) << stage;
                    int box_low = box + stride;
                    int box_hig = box + (1<<stage)-stride-1;
                    if (i < box_low || i >= box_hig) partner = i;
                    else partner = ((i/stride-box/stride)%2) ? i + stride : i - stride;
                    //printf("    %d <-> %d b:%d-%d (%d-%d) st:%d\n",i,partner,box,box+(1<<stage)-1,box_low,box_hig,stride);
                }
                if (partner > i && partner < n) {
                    cmp_swap(i,partner);
                    YIELD(1);

                }
            }
        }
    }
    return 1;
}

static int coctail_shaker_sort(){
    CONTINUE(1);
    CONTINUE(2);
    start = 0;
    end = n-1;
    while(1) {
        inversions = 0;
        for (i=start;i<end;i++) {
            if (data[i] > data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(1);
        }
        end = last;
        if (!inversions) break;
        inversions = 0;
        for (i=end-1;i>=start;i--){
            if (data[i] > data[i+1]) last = i;
            cmp_swap(i,i+1);
            YIELD(2);
        }
        start = last;
        if (!inversions) break;
    }
    return 1;
}

static int heap_sort(){
    CONTINUE(1);
    CONTINUE(2);
    //CONTINUE(3);
    CONTINUE(4);
    CONTINUE(5);

    // heapify
    for (start=n/2; start>=0;start--) {
        end = n-1;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(1);
            if (data[swapable] < data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(2);
                if (data[swapable] < data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    // heapsort
    for (end=n-1;end > 0;) {
        swap(end,0);
        //h1=0;h2=end;YIELD(3);
        end--;
        start = 0;
        root = start;
        // sift down
        while (2*root+1<=end){
            child = 2*root+1;
            swapable = root;
            h1 = swapable; h2 = child; YIELD(4);
            if (data[swapable] < data[child]) swapable = child;
            if (child +1 <= end) {
                h1 = swapable; h2 = child+1; YIELD(5);
                if (data[swapable] < data[child+1]) swapable = child+1;
            }
            if (swapable == root) break;
            else {
                swap(swapable,root);
                root = swapable;
            }
        } 
    }
    return 1;
}


static int bubblesort() {
    CONTINUE(1);
    for (j = n-1; j>0; j--) {
        inversions = 0;
        for (i = 0; i<j; i++) {
            cmp_swap(i,i+1);
            YIELD(1);
        }
        if (inversions == 0) {
            return 1;
        }
    }
    return 1;
}

static int selection_sort2() {
    CONTINUE(1);
    for (j = 0; j < n-1; j++) {
        for (i = j+1; i < n; i++) {
            cmp_swap(j,i);
            YIELD(1);
        }
    }
    return 1;
}

static int selection_sort3() {
    CONTINUE(1);
    CONTINUE(2);
    for (j = 0; j < (n-1)/2; j++) {
        cmp_swap(j+1,n-1-j);
        for (i = j+1; i < n-1-j; i++) {
            cmp_swap(j,i);
            YIELD(2);
            cmp_swap(i,n-1-j);
            YIELD(1);
        }
    }
    return 1;
}

static int selection_sort() {
    CONTINUE(1);
    CONTINUE(2);
    for (j = 0; j < n-1; j++) {
        iMin = j;
        h1 = iMin;
        for (i = j+1; i < n; i++) {
            h2 = i;
            YIELD(1);
            if (data[i] < data[iMin]) {
                iMin = i;
                h1 = iMin;
            }
        }
        h2 = j;
        swap(j, iMin);
        YIELD(2);
    }
    return 1;
}

// Like it's inverse bubblesort, but worse
static int tournament_sort(){
    CONTINUE(1);
    for (j=0;j<n-1;j++){
        for (stride = 1;stride +j < n;stride *=2){
            for (i=j;i+stride<n;i+=stride*2){
                cmp_swap(i,i+stride);
                YIELD(1);
            }
        }
    }
    return 1;
}

static int insertion_sort() {
    CONTINUE(1);
    for (i=1; i<n; i++) {
        for (j=i; j>0&&data[j-1]>data[j]; j--) {
            cmp_swap(j-1,j);
            YIELD(1);
        }
    }
    return 1;
}

static int comb_sort() {
    CONTINUE(1);
    step = n;
    do {
        inversions = 0;
        step = (int)floor(step/1.3);
        if (step == 0) step=1;
        for (i=0; i+step<n; i++) {
            cmp_swap(i,i+step);
            YIELD(1);
        }
    } while (step > 0 && inversions);
    return 1;
}

const static int shellsort_gaps[] = {701,301,132,57,23,10,4,1}; 

static int shell_sort() {
    CONTINUE(1);
    for(stage=0;stage<8;stage++){
        step = shellsort_gaps[stage];
        for (i=1; i<n; i++) {
            for (j=i; j-step>=0&&data[j-step]>data[j]; j-=step) {
                cmp_swap(j-step,j);
                YIELD(1);
            }
        }
    }
    return 1;
}


static int compare_int (const void * a, const void * b){return ( *(int*)a - *(int*)b );}
static int shuffle(){
    CONTINUE(1);
    qsort(data,n,sizeof(int),compare_int);
    data[0] = 1;
    for (i=1; i<n; i++) {
        partner = randn(i);
        data[i] = data[partner];
        data[partner] = i+1;
        YIELD(1);
    }
    return 1;
}

static int quick_sortLL(){
    CONTINUE(1);
    CONTINUE(2);
    data2[0] = 0;
    data2[1] = n-1;
    stack_p = 2;
    while(stack_p > 0){
        ll = data2[stack_p-2];
        rr = data2[stack_p-1];
        stack_p -= 2;
        if (rr - ll <= 0) continue;
        l=ll;
        for(r=ll;r<rr;r++){
            h1=l;h2=r;
            if(data[r] < data[rr]){
                swap(l,r);
                l++;
            }
            YIELD(1);
        }
        swap(l,rr);
        if (stack_p+4 < n){
            if (l-ll > rr -l){
                data2[stack_p+0] = ll;
                data2[stack_p+1] = l-1;
                data2[stack_p+2] = l+1;
                data2[stack_p+3] = rr;
                stack_p += 4;
            } else {
                data2[stack_p+0] = l+1;
                data2[stack_p+1] = rr;
                data2[stack_p+2] = ll;
                data2[stack_p+3] = l-1;
                stack_p += 4;
            }
        // Change to Insertion Sort, because stack space ran out
        // Shouldnt happen!
        } else {
            for (i=1; i<n; i++)for (j=i; j>0&&data[j-1]<data[j]; j--)
            {cmp_swap(j-1,j);YIELD(2);}
            return 1;
        }
    }
    return 1;
}

static int quick_sortLR(){
    CONTINUE(1);
    CONTINUE(2);
    data2[0] = 0;
    data2[1] = n-1;
    stack_p = 2;
    while(stack_p > 0){
        ll = data2[stack_p-2];
        rr = data2[stack_p-1];
        stack_p -= 2;
        if (rr - ll <= 0) continue;
        // partition
        for(h1=l=ll,h2=r=rr;l<r;){
            YIELD(1);
            if (data[r] > data[ll]){
                h2=--r;continue;
            }
            if (data[l] <= data[ll]){
                h1=++l;continue;
            }
            swap(l,r);
        }
        swap(ll,l);
        // end partition
        if (stack_p+4 < n){
            if (l-ll > rr -l){
                data2[stack_p+0] = ll;
                data2[stack_p+1] = l-1;
                data2[stack_p+2] = l+1;
                data2[stack_p+3] = rr;
                stack_p += 4;
            } else {
                data2[stack_p+0] = l+1;
                data2[stack_p+1] = rr;
                data2[stack_p+2] = ll;
                data2[stack_p+3] = l-1;
                stack_p += 4;
            }
        // Change to Insertion Sort, because stack space ran out
        // Shouldnt happen!
        } else {
            for (i=1; i<n; i++)for (j=i; j>0&&data[j-1]<data[j]; j--)
            {cmp_swap(j-1,j);YIELD(2);}
            return 1;
        }
    }
    return 1;
}

static int merge_sort_inplace(){
    CONTINUE(1);
    for (step=1;step<n;step*=2){
        for (i=0;i+step<n;i+=2*step){
            l=i;
            r=i+step;
            for (j=i;(j<i+2*step)&&(j<n)&&(l<r)&&(r<i+2*step);j++){
                h1=l;h2=r;
                YIELD(1);
                if (data[l] < data[r]){
                    l++; continue;
                }
                last=data[r];
                for (ll=r-1;ll>=l;ll--) data[ll+1]=data[ll];
                data[l]=last;
                r++; l++;
            }
        }
    }
    return 1;
}


static int radix_sort_LSD(){
    CONTINUE(1);
    for (stage = 0;stage<logn;stage+=2){
        a=0;b=0;c=0;
        for(i=0;i<n;i++){
            last = data[i];
            h1=i;
            switch ((last>>stage)&0x3) {
                case 0:
                    h2=a;
                    for (ll=i-1;ll>=a;ll--) data[ll+1]=data[ll];
                    data[a]=last;
                    a++;b++;c++;
                    break;
                case 1:
                    h2=b;
                    for (ll=i-1;ll>=b;ll--) data[ll+1]=data[ll];
                    data[b]=last;
                    b++;c++;
                    break;
                case 2:
                    h2=c;
                    for (ll=i-1;ll>=c;ll--) data[ll+1]=data[ll];
                    data[c]=last;
                    c++;
                    break;
                case 3:
                    h2=i;
                    break;
            }
            YIELD(1);
        }
    }
    return 1;
}

static int pred_bubblesort(){ return n*n/2;}
static int pred_comb_sort(){ return n*logn*2;}
static int pred_insertion_sort(){return n*n/4;}
static int pred_selection_sort(){return n*n/2;}
static int pred_heap_sort(){return n*logn*3/2;}
static int pred_coctail_shaker_sort(){ return n*n/3;}
static int pred_odd_even_mergesort(){ return n*logn*logn/4;}
static int pred_bitonic_sort(){ return n*logn*logn/3.5;}
static int pred_pairwise_sorting_net(){ return n*logn*logn/4;}
static int pred_shell_sort(){return n*logn/1.66;}
static int pred_selection_sort2(){ return n*n/2;}
static int pred_selection_sort3(){ return n*n/2;}
static int pred_tournament_sort(){ return n*n/2;}
static int pred_shuffle(){return n;}
static int pred_quick_sort(){return n*logn;}
static int pred_merge_sort(){return n*logn;}




static int sort() {
    switch (sorting_algorithm) {
    case 0: return bubblesort(); // n*n/2
    case 1: return comb_sort(); // n*log(n)*2
    case 2: return insertion_sort(); // n*n/4
    case 3: return selection_sort(); // n*n/2
    case 4: return heap_sort();       // n*log(n)*1.5
    case 5: return coctail_shaker_sort(); //n*n /3 
    case 6: return odd_even_mergesort(); //n*log(n)*1.5
    case 7: return bitonic_sort(); //n*log(n)*1.5
    case 8: return pairwise_sorting_net(); //n&log(n)*15
    case 9: return shell_sort();
    case 10: return selection_sort2();
    case 11: return selection_sort3();
    case 12: return tournament_sort(); //n*n/2*log(n)
    case 13: return shuffle();
    case 14: return quick_sortLL();
    case 15: return quick_sortLR();
    case 16: return merge_sort_inplace();
    case 17: return radix_sort_LSD();
    default: return bubblesort();
    }
}

static int predict(int sorting_algorithm){
    for (logn=1;1<<logn<n;logn++);
    switch (sorting_algorithm) {
    case 0: return pred_bubblesort(); // n*n/2
    case 1: return pred_comb_sort(); // n*log(n)*2
    case 2: return pred_insertion_sort(); // n*n/4
    case 3: return pred_selection_sort(); // n*n/2
    case 4: return pred_heap_sort();       // n*log(n)*1.5
    case 5: return pred_coctail_shaker_sort(); //n*n /3 
    case 6: return pred_odd_even_mergesort(); //n*log(n)*1.5
    case 7: return pred_bitonic_sort(); //n*log(n)*1.5
    case 8: return pred_pairwise_sorting_net(); //n&log(n)*15
    case 9: return pred_shell_sort();
    case 10: return pred_selection_sort2();
    case 11: return pred_selection_sort3();
    case 12: return pred_tournament_sort(); //n*n/2*log(n)
    case 13: return pred_shuffle();
    case 14: return pred_quick_sort();
    case 15: return pred_quick_sort();
    case 16: return pred_merge_sort();
    case 17: return n*logn;
    default: return pred_bubblesort();
    }
}

