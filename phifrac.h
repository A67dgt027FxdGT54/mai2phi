#pragma once

unsigned int getXBitsInFront(unsigned int i,int x){
	if(x<0) return 0;
	int bitsInTotal = 0;
	for(int l=31;l>=0;l--){
		if(i & (1u << l)){
			bitsInTotal = l + 1;
			break;
		}
	}
	if(x > bitsInTotal) return 0;
	return i >> (bitsInTotal - x);
}
unsigned int getXBitsAtBack(unsigned int i,int x){
	if(x<=0) return 0;
	return i & ((1u << x) - 1);
}

struct phifrac{
	long long integer,p,q;
	phifrac():integer(0),p(0),q(1){
	}
	
	phifrac(int i):integer(i),p(0),q(1){
	}
	phifrac(float _f){ 
		// 1 - 8 - 23
		unsigned int i;
		std::memcpy(&i, &_f, sizeof i);
		int sign = (i>>31) ? -1 : 1;
		int e = (i>>23) & 0xFF;
		unsigned int f = (i & 0x7FFFFF);  
		
		if(e==0 && f==0){
			integer = 0;
			p = 0;
			q = 1;
			return;
		}
		if(e==0xFF && f==0){ // inf
			integer = sign * 0x7FFFFFFFFFFFFFFF; 
			p = 0;
			q = 1;
			return;
		}
		if(e==0xFF && f!=0){ // NaN
			integer = 0;
			p = 0;
			q = 1;
			return;
		}
		if(e==0 && f!=0){
			e = 1-127;
		}
		else{
			e = e-127;
			f |= (1<<23);
		}
		if(e>=23){
			if(e < 31) integer = sign * (signed)(f << (e-23));
			else integer = sign * (signed)0x7FFFFFFFFFFFFFFF;
			p = 0;
			q = 1;
		}
		else if(e>=0){
			integer = sign * (signed)getXBitsInFront(f,e+1);
			p = sign * (signed)getXBitsAtBack(f,23-e);
			q = 1<<(23-e);
		}
		else{
			integer = 0;
			if(23-e < 31)
				p = sign * (signed)f,
				q = 1<<(23-e);
			else{
				int ofl = 23 - e - 30;
				p = sign * (ofl >= 32 ? 0 : (signed)(f >> ofl));
				q = 1<<30;
			}
		}
	}
	
	phifrac(const phifrac& o) = default;
	phifrac& operator =(const phifrac& o) = default;
	phifrac(phifrac&& o) = default;
	phifrac& operator =(phifrac&& o) = default;
};

long long abs(long long x){
	if(x<0) return -x;
	return x;
}
long long gcd(long long x,long long y){
	x = abs(x);
	y = abs(y);
	if(y==0) return x;
	return gcd(y,x%y);
}

phifrac normalize(phifrac x){
	long long g = gcd(x.p, x.q);
	x.p /= g;
	x.q /= g;
	x.integer += x.p / x.q;
	x.p = x.p % x.q;
	return x;
}
phifrac operator +(phifrac x){
	return normalize(x);
}
phifrac operator -(phifrac x){
	x.integer = -x.integer;
	x.p = -x.p;
	return normalize(x);
}
phifrac operator +(phifrac x,phifrac y){
	phifrac res;
	res.integer = x.integer + y.integer;
	res.q = x.q * y.q;
	res.p = x.p * y.q + x.q * y.p;
	return normalize(res);
}
phifrac operator -(phifrac x,phifrac y){
	return normalize(x + -y);
}
phifrac operator *(phifrac x,phifrac y){
	phifrac res;
	res.integer = x.integer*y.integer;
	res.q = x.q * y.q;
	res.p = x.integer * x.q * y.p + y.integer * x.p * y.q + x.p * y.p;
	return normalize(res);
}
phifrac inverse(phifrac x){
	phifrac res;
	res.integer = 0;
	res.p = x.q;
	res.q = x.integer * x.q + x.p;
	return normalize(res);
}
phifrac operator /(phifrac x,phifrac y){
	return normalize(x * inverse(y));
}
phifrac operator %(phifrac x,phifrac y){
	phifrac res = x / y;
	res.integer = 0;
	return res;
}

std::ostream& operator << (std::ostream& os,const phifrac& x){
	return os << x.integer << " " << x.p << "/" << x.q;
}




