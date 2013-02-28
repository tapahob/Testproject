cbuffer cbNeverChanges : register( b0 )
{
    matrix View;
};

cbuffer cbChangeOnResize : register( b1 )
{
    matrix Projection;
};

cbuffer cbChangesEveryFrame : register( b2 )
{
    matrix World;
};


struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
	output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Tex = input.Tex;
    
    return output;
}

PS_INPUT VS_Bullet( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    
    return output;
}

float4 PS( PS_INPUT input) : SV_Target
{

	if ( length( input.Tex) < 0.5f)
	{
		return float4(1.0f,1.0f,1.f,1.0f);
	}

	return float4(0.0f,0.0f,0.0f,1.0f);
}

float4 PS_Bullet( PS_INPUT input) : SV_Target
{
	return float4(1.0f,1.0f,1.f,1);
}
