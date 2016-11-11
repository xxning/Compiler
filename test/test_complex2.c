//求整数的因子的测试程序
//其中包含while和if的嵌套使用,数组使用,变量声明等.
int N;
void main(){
	
	int i=2,j=1;
	while(i<=N){
		if(N%i==0)
		j=j+1;
		i=i+1;
	}
	int fac[100];
	fac[0]=1;
	int k=1;
	i=2;
	while(i<=N){
		if(N%i==0){
			fac[k]=i;
			k=k+1;
		}
		i=i+1;
	}
	
	i=0;
	while(i!=j){
		i=i+1;
	}
	
}
