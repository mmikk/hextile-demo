#ifndef __NOISE_H__
#define __NOISE_H__

Buffer<uint> g_uPermTable;
Buffer<float3> g_v3GradArray;




/*********************************************************************************************
************************************* Utility functions **************************************
*********************************************************************************************/

float3 fade(float3 t)
{
	return t*t*t*(t*(t*6-float3(15, 15, 15))+float3(10, 10, 10));	 // new curve
 // return t * t * (3 - 2 * t); // old curve
}

float3 dfade(float3 t)
{	
	return 30*t*t*(t*(t-float3(2, 2, 2))+float3(1, 1, 1));  // new curve
  //return 6*t*(1-t); // old curve
}


int perm(int x)
{
	return g_uPermTable[x&255];
  //return round( tex1D(permSampler, float2((x+0.5) / 256.0, 0)).x * 255.0 );
  //return round( tex1D(permSampler, (x+0.5) / 256.0).x * 255.0 );
 
  //return round( permTexture.Sample(samPoint, (x+0.5) / 256.0).x * 255.0 );
}

float3 get_grad(int x)
{
    return g_v3GradArray[x&15];
	//return 2*gradTexture.Sample(samPoint, (x+0.5) / 16.0).xyz - float3(1,1,1);
}

float grad(int x, float3 p)
{
	float3 vGrad = get_grad(x);
	//float3 vGrad = 2*tex1D(gradSampler, float2((x+0.5) / 16.0,0)).xyz - float3(1,1,1);
  	return dot(vGrad, p);
}

/*********************************************************************************************
**************************************** API functions ***************************************
*********************************************************************************************/

float GetPixelSize(float3 vP)
{
	return sqrt(length( cross(ddx(vP), ddy(vP)) ));
	//return max(length(ddx(vP)), length(ddy(vP)));
}

float mysnoise(float3 p)
{
	int3 P = floor(p);
	p -= P;
	P &= int3(255,255,255);
	
  //int3 P = fmod(floor(p), 256.0);
  //p -= floor(p);
  
  float3 f = fade(p);

  // HASH COORDINATES FOR 6 OF THE 8 CUBE CORNERS
  int A = perm(P.x) + P.y;
  int AA = perm(A) + P.z;
  int AB = perm(A + 1) + P.z;
  int B =  perm(P.x + 1) + P.y;
  int BA = perm(B) + P.z;
  int BB = perm(B + 1) + P.z;


  // AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
  return lerp(
    lerp(lerp(grad(perm(AA), p),
              grad(perm(BA), p + float3(-1, 0, 0)), f.x),
         lerp(grad(perm(AB), p + float3(0, -1, 0)),
              grad(perm(BB), p + float3(-1, -1, 0)), f.x), f.y),
    lerp(lerp(grad(perm(AA + 1), p + float3(0, 0, -1)),
              grad(perm(BA + 1), p + float3(-1, 0, -1)), f.x),
         lerp(grad(perm(AB + 1), p + float3(0, -1, -1)),
              grad(perm(BB + 1), p + float3(-1, -1, -1)), f.x), f.y),
    f.z);
}

float4 mydsnoise(float3 p)
{
	int3 P = floor(p);
	p -= P;
	P &= int3(255,255,255);
	//int3 P = fmod(floor(p), 256.0);
  	//p -= floor(p);

	float3 F = fade(p);
	float3 dF = dfade(p);
	

	// HASH COORDINATES FOR 6 OF THE 8 CUBE CORNERS
	int A = perm(P.x) + P.y;
	int AA = perm(A) + P.z;
	int AB = perm(A + 1) + P.z;
	int B =  perm(P.x + 1) + P.y;
	int BA = perm(B) + P.z;
	int BB = perm(B + 1) + P.z;
  

  // AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
  	float4 vA = float4(get_grad(perm(AA)), 0);
	float4 vB = float4(get_grad(perm(BA)), 0);
	float4 vC = float4(get_grad(perm(AB)), 0);
	float4 vD = float4(get_grad(perm(BB)), 0);
	float4 vE = float4(get_grad(perm(AA+1)), 0);
	float4 vF = float4(get_grad(perm(BA+1)), 0);
	float4 vG = float4(get_grad(perm(AB+1)), 0);
	float4 vH = float4(get_grad(perm(BB+1)), 0);
  
	vA.w = dot(vA.xyz, p);
	vB.w = dot(vB.xyz, p + float3(-1, 0, 0));
	vC.w = dot(vC.xyz, p + float3(0, -1, 0));
	vD.w = dot(vD.xyz, p + float3(-1, -1, 0));
	vE.w = dot(vE.xyz, p + float3(0, 0, -1));
	vF.w = dot(vF.xyz, p + float3(-1, 0, -1));
	vG.w = dot(vG.xyz, p + float3(0, -1, -1));
	vH.w = dot(vH.xyz, p + float3(-1, -1, -1));
	
	const float u = F.x;
	const float v = F.y;
	const float w = F.z;

  	/*const float noise = 
  		lerp(	
  				lerp(	lerp(a,b,u), lerp(c,d,u), 	v),
    			lerp(	lerp(e,f,u), lerp(g,h,u),	v),
				w);*/
		
	
	/*const float4 vK0 = vA;
	const float4 vK1 = vB-vA;
	const float4 vK2 = vC-vA;
	const float4 vK3 = vE-vA;
	const float4 vK4 = vA-vB-vC+vD;
	
	const float4 vK5 = vA-vC-vE+vG;
	const float4 vK6 = vA-vB-vE+vF;
	const float4 vK7 = -vA+vB+vC-vD+vE-vF-vG+vH;
	
	
	float4 dNoise = vK0 + u*vK1 + v*vK2 + w*vK3 + u*v*vK4 + v*w*vK5 + u*w*vK6 + u*v*w*vK7;
	
	dNoise.x += dF.x*(vK1.w + vK4.w*v + vK6.w*w + vK7.w*v*w);
	dNoise.y += dF.y*(vK2.w + vK5.w*w + vK4.w*u + vK7.w*w*u);
	dNoise.z += dF.z*(vK3.w + vK6.w*u + vK5.w*v + vK7.w*u*v);*/
	
	
	
	float4 vK0 = vA;
	float4 vK1 = vB-vA;
	float4 vK2 = vC-vA;
	float4 vK3 = vE-vA;
	float4 vK4 = vD-vK1-vC;
	float4 vK5 = vG-vK2-vE;
	float4 vK6 = vF-vK1-vE;
	float4 vK7 = vE-vF-vG+vH-vK4;
	
	vK0.xyz += dF*float3(vK1.w, vK2.w, vK3.w);
	vK1.yz += dF.yz * float2(vK4.w, vK6.w);
	vK2.xz += dF.xz * float2(vK4.w, vK5.w);
	vK3.xy += dF.xy * float2(vK6.w, vK5.w);
	vK4.z += dF.z*vK7.w;
	vK5.x += dF.x*vK7.w;
	vK6.y += dF.y*vK7.w;

	float4 dNoise = vK0 + u*vK1 + v*(vK2 + u*vK4) + w*(vK3 + u*vK6 + v*(vK5 + u*vK7));
	
	
	// (height, gradient)
	return float4(dNoise.w, dNoise.xyz);
}

float mynoise(float3 p)
{
	return 0.5*mysnoise(p)+0.5;
}

float4 mydnoise(float3 p)
{
	return 0.5*mydsnoise(p)+float4(0.5,0,0,0);
}


//dNoise.w = dot(dNoise.xyz, p)
	//			- u*vB.x - v*vC.y - w*vE.z +
	//			u*v*(vB.x+vC.y-vD.x-vD.y) + v*w * (vC.y+vE.z-vG.y-vG.z) + u*w * (vB.x+vE.z-vF.x-vF.z) +
	//			u*v*w * (-vB.x-vC.y+vD.x+vD.y-vE.z+vF.x+vF.z+vG.y+vG.z-vH.x-vH.y-vH.z);

// TURBULENCE TEXTURE
float VerifyPixSize(float fPixSize)
{
	// return fPixSize;
	return max(fPixSize, 1.0/(1<<24));
}

float filteredsnoise(float3 p, float width)
{
	return mysnoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float4 filtereddsnoise(float3 p, float width)
{
	return mydsnoise(p) * (1 - smoothstep(0.25,0.75,width));
}

float filteredabs(float x, float dx)
{
	float x0 = x-0.5*dx;
	float x1 = x+0.5*dx;
	return (sign(x1)*x1*x1 - sign(x0)*x0*x0) / (2*dx);
}


// FRACTAL SUM
#define NEW_METHOD

float fractalsum(float3 P, float fPixSize_in=1)
{
	float fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		fracsum += (1/freq) * filteredsnoise(P*freq, fw);
		freq*=2; fw *= 2;
	}
#else
   	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		fracsum += scale * mysnoise(P/scale);

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		fracsum += weight * scale * mysnoise(P/scale);
   }
#endif

	return fracsum;
	
}


float4 dfractalsum(float3 P, float fPixSize_in=1)
{
	float4 fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		fracsum += float4((1/freq),1,1,1) * filtereddsnoise(P*freq, fw);
		freq*=2; fw *= 2;
	}
#else
	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		fracsum += float4(scale,1,1,1) * mydsnoise(P/scale);

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		fracsum += weight * float4(scale,1,1,1) * mydsnoise(P/scale);
   }
#endif

	return fracsum;
}

float turbulence(float3 P, float fPixSize_in=1)
{
	float turbulence = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
#ifdef NEW_METHOD
	float freq = 1;
	float fw = fPixSize;	// should clamp this
	while(fw < 1.0)
	{
		turbulence += (1/freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
		freq*=2; fw *= 2;
	}
#else
   	const float twice = 2*fPixSize;
	float scale = 1;
   	for(scale = 1; scale > twice; scale /= 2) 
    		turbulence += scale * abs(mysnoise(P/scale));

   /* Gradual fade out of highest-frequency component near limit */
   if (scale > fPixSize)
   {
		float weight = (scale / fPixSize) - 1;
		weight = clamp(weight, 0, 1);
		turbulence += weight * scale * abs(mysnoise(P/scale));
   }
#endif

	return turbulence;
}

float4 dturbulence(float3 P, float fPixSize_in=1)
{
	float pixSize = VerifyPixSize(fPixSize_in);

	float Ncen = turbulence( P, pixSize );
	float Nrgt = turbulence( float3(P.x+pixSize, P.yz), pixSize );
	float Nup = turbulence( float3(P.x,P.y+pixSize, P.z), pixSize );
	float Ndpth = turbulence( float3(P.xy,P.z+pixSize), pixSize );

	float3 grad = (1.0/pixSize)*float3(Nrgt-Ncen, Nup-Ncen, Ndpth-Ncen);

	return float4(Ncen, grad.xyz);
}

// MAX versions. As an optimization only the last (highest) frequencies are accumulated
// this is sufficient for derivative calculations used for normal perturbation.
float fractalsummax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);
	
	//1/(2^i) = t
	//fI = -log_2(t)	// ceil is break
	//i = ceil(fI)
	//scale = 1/(2^i) = pow(2,-i)
	//scale = pow(2,-ceil(-log_2(t))
	//	  = pow(2,floor(log_2(t))

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			fracsum += (1/freq) * filteredsnoise(P*freq, fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			fracsum += ((1-weight) / freq) * filteredsnoise(P*freq, fw);
		}
	
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		fracsum += (weight / freq) * mysnoise(P*freq);
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			fracsum += mysnoise(P*freq) / freq;
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			fracsum += ((1-weight) / freq) * mysnoise(P*freq);
		}
		//assert(scale>=1);
#endif
	}
	return fracsum;
}

float4 dfractalsummax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float4 fracsum = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			fracsum += float4(1.0/freq,1,1,1) * filtereddsnoise(P*freq, fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			fracsum += (1-weight) * float4(1.0/freq,1,1,1) * filtereddsnoise(P*freq, fw);
		}
	
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		fracsum += weight * float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			fracsum += float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			fracsum += (1-weight) * float4(1.0/freq,1,1,1) * mydsnoise(P*freq);
		}
		//assert(scale>=1);
#endif
	}

	return fracsum;
}

float turbulencemax(float3 P, float fPixSize_in=1, int iMaxOctaves=8)
{
   	float turbulence = 0;
	float fPixSize = VerifyPixSize(fPixSize_in);

	float fK = -floor(log2(fPixSize));
	int iPermOctaves = (int) fK;
	if(iPermOctaves>0)
	{
		float freq = pow(2.0,fK-1);	// scale is an integer power of two
		const float weight = saturate((1 / (freq*fPixSize)) - 1);
#ifdef NEW_METHOD
		float fw = fPixSize*freq;	// should clamp this
		const int iIterations = min(iPermOctaves, iMaxOctaves-1);
		for(int k=0; k<iIterations; k++)
		{
			turbulence += (1/freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
			freq/=2; fw /= 2;
		}
		if(iPermOctaves>=iMaxOctaves)
		{
			turbulence += ((1-weight) / freq) * filteredabs( filteredsnoise(P*freq, fw), 2*fw);
		}
#else
		const int iIterations = min(iPermOctaves-1, iMaxOctaves-2);
		turbulence += (weight / freq) * abs(mysnoise(P*freq));
		for(int k=0; k<iIterations; k++)
		{
			freq /= 2;
			turbulence += abs(mysnoise(P*freq)) / freq;
		}
		
		if(iPermOctaves>=iMaxOctaves)
		{
			freq /= 2;
			turbulence += ((1-weight) / freq) * abs(mysnoise(P*freq));
		}
		//assert(scale>=1);
#endif
	}

	return turbulence;
}


float4 dturbulencemax(float3 P, float fPixSize_in=1)
{
	float pixSize = VerifyPixSize(fPixSize_in);

	float Ncen = turbulencemax( P, pixSize );
	float Nrgt = turbulencemax( float3(P.x+pixSize, P.yz), pixSize );
	float Nup = turbulencemax( float3(P.x,P.y+pixSize, P.z), pixSize );
	float Ndpth = turbulencemax( float3(P.xy,P.z+pixSize), pixSize );

	float3 grad = (1.0/pixSize)*float3(Nrgt-Ncen, Nup-Ncen, Ndpth-Ncen);

	return float4(Ncen, grad.xyz);
}


#endif