int a = 1;
int b = -2147483647;
int c ;
int main() {                      
    goto d;                      
    e:     
        a = 0;                      
    f:     
        if ( -2147483647 * a -2147483647 * c >= 0) goto g;    
    h:     
        goto i;                      
    d:     
        if (  b >= 0) goto e;                          
        goto j;                      
    i:     
        if (  c -2147483647 * a >= 0) goto h;
        goto d;                      
    j:     
        b = 7;
        goto f;                      
    g:     
    return 0;                      
}
