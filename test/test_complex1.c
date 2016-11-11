//这是一个C1语言的复杂测试程序
//包括函数定义,变量常量定义,while/if语句,函数调用,变量赋值,数组使用等
//功能是实现数组的排序
int N=5;
int a[5];
const int s=1+2;
void sort(){

	int i=0,j,tem;
	while(i!=N){
		j=i+1;
		while(j!=N){
			
			if(a[j]<a[i]){
				tem=a[j];
				a[j]=a[i];
				a[i]=tem;
			}
			j=j+1;
		}
		i=i+1;
	}

}

void main(){
      
	int i=0;	
	while(i!=N){
		i=i+1;
	}
		
	sort();
	i=0;
	while(i!=N){
		i=i+1;
	}

		
}
