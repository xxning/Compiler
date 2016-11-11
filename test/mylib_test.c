//用于lib库的测试
int Output=10;
void print();
void printi();
void main() {
    //int foo=10;

    int a[2] = {10,6};
    while (a[1] > 0) {
        Output = a[1];
        //print();
	printi();
	if(!a[1]>3) {break;}
        a[1] = a[1] - 1;
    }   
  
}
