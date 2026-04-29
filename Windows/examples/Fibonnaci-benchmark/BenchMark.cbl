extern def printf(fmt: *void, ...) -> int
extern def clock() -> int

def fib_fast(n: int) -> int:
    if n <= 1:
        return n
    
    a: int = 0
    b: int = 1
    i: int = n
    
    while i >= 4:
        a = a + b
        b = b + a
        a = a + b
        b = b + a
        i = i - 4
        
    while i > 1:
        t: int = a + b
        a = b
        b = t
        i = i - 1
        
    return b

def main() -> int:
    start: int = clock()
    
    iter: int = 0
    total_sum: int = 0
    
    while iter < 100000000:
        total_sum = total_sum + fib_fast(iter % 41)
        iter = iter + 1
    
    end: int = clock()
    time: int = end - start
    
    printf("Check sum: %d\n".data as *void, total_sum)
    printf("100m запусков выдало: %d ms\n".data as *void, time)
    
    return 0