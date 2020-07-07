#ifndef SHADER_H
#define SHADER_H


constexpr const char* D3D11FillShader = R"(

struct VSOut
{
	float4 Col : COLOR;
	float4 Pos : SV_POSITION;
};

VSOut VS(float4 Col : COLOR, float4 Pos : POSITION)
{
	VSOut Output;
	Output.Pos = Pos;
	Output.Col = Col;
	return Output;
}

float4 PS(float4 Col : COLOR) : SV_TARGET
{
	return Col;
}

)";

#endif 