cbuffer global
{
    matrix gWVP;
};

struct GS_IN
{
    float4 Pos : POSITION;
};

struct GS_OUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};
 
GS_IN VS(float4 Pos : POSITION)
{
    GS_IN Out;
    Out.Pos = Pos;
    return Out;
}
 
float4 PS(GS_OUT In) : SV_Target
{
    return In.Color;
}

[maxvertexcount(101)]
void GS(triangle GS_IN gs_in[3], inout LineStream<GS_OUT> stream)
{
    GS_OUT Out;

    for (float Point = 0.0f; Point <= 1.0f; Point += 0.01f)
    {
        Out.Color = float4(Point, 1.0f - Point, 0, 1);
        float4 left = Point * gs_in[0].Pos + (1.0f - Point) * gs_in[1].Pos;
        float4 right = Point * gs_in[1].Pos + (1.0f - Point) * gs_in[2].Pos;
        Out.Pos = Point * left + (1.0f - Point) * right;
        Out.Pos = mul(Out.Pos, gWVP);
        stream.Append(Out);
    }
    stream.RestartStrip();
}