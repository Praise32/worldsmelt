#version 330

// Un solo present shader per confrontare tre rese della STESSA simulazione:
//   mode < 0.5  -> pixel: campionamento a blocchi + palette piu discreta
//   mode < 1.5  -> smooth: immagine continua + glow e color grading
//   altrimenti  -> ibrido: silhouette nitida/pixel + luce continua
// Il gameplay non entra mai nello shader: riceve soltanto il RenderTexture.

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float mode;
uniform float pixelSize;
uniform float time;

out vec4 finalColor;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 brightPass(vec3 color)
{
    float gate = smoothstep(0.48, 0.92, luminance(color));
    return color*gate;
}

vec3 localGlow(vec2 uv, vec2 texel)
{
    vec3 glow = brightPass(texture(texture0, uv).rgb)*0.18;

    glow += brightPass(texture(texture0, uv + vec2( texel.x, 0.0)).rgb)*0.10;
    glow += brightPass(texture(texture0, uv + vec2(-texel.x, 0.0)).rgb)*0.10;
    glow += brightPass(texture(texture0, uv + vec2(0.0,  texel.y)).rgb)*0.10;
    glow += brightPass(texture(texture0, uv + vec2(0.0, -texel.y)).rgb)*0.10;

    glow += brightPass(texture(texture0, uv + vec2( texel.x,  texel.y)*3.0).rgb)*0.07;
    glow += brightPass(texture(texture0, uv + vec2(-texel.x,  texel.y)*3.0).rgb)*0.07;
    glow += brightPass(texture(texture0, uv + vec2( texel.x, -texel.y)*3.0).rgb)*0.07;
    glow += brightPass(texture(texture0, uv + vec2(-texel.x, -texel.y)*3.0).rgb)*0.07;

    glow += brightPass(texture(texture0, uv + vec2( texel.x, 0.0)*7.0).rgb)*0.035;
    glow += brightPass(texture(texture0, uv + vec2(-texel.x, 0.0)*7.0).rgb)*0.035;
    glow += brightPass(texture(texture0, uv + vec2(0.0,  texel.y)*7.0).rgb)*0.035;
    glow += brightPass(texture(texture0, uv + vec2(0.0, -texel.y)*7.0).rgb)*0.035;

    return glow;
}

vec3 pixelPalette(vec3 color, vec2 pixelCoord)
{
    // Dither 2x2 stabile: nessun rumore temporale e quindi nessuna variazione
    // del replay. I livelli non impongono uno stile specifico al progetto.
    float pattern = mod(pixelCoord.x + 2.0*mod(pixelCoord.y, 2.0), 4.0);
    float dither = (pattern - 1.5)/48.0;
    vec3 levels = vec3(10.0, 11.0, 10.0);
    return floor(clamp(color + dither, 0.0, 1.0)*levels + 0.5)/levels;
}

vec3 grade(vec3 color, vec2 uv)
{
    float luma = luminance(color);
    vec3 shadowTint = vec3(0.90, 0.97, 1.08);
    vec3 highlightTint = vec3(1.07, 1.01, 0.93);
    color *= mix(shadowTint, highlightTint, smoothstep(0.18, 0.82, luma));

    vec2 centered = uv*2.0 - 1.0;
    float vignette = 1.0 - 0.16*smoothstep(0.48, 1.32, dot(centered, centered));
    return color*vignette;
}

void main()
{
    vec2 textureExtent = vec2(textureSize(texture0, 0));
    vec2 texel = 1.0/max(textureExtent, vec2(1.0));
    float block = max(pixelSize, 1.0);
    vec2 snappedUv = (floor(fragTexCoord/(texel*block)) + 0.5)*(texel*block);

    vec4 raw = texture(texture0, fragTexCoord);
    vec4 crisp = texture(texture0, snappedUv);
    vec3 color;

    if (mode < 0.5)
    {
        color = pixelPalette(crisp.rgb, floor(gl_FragCoord.xy/block));
    }
    else
    {
        float pulse = 0.96 + 0.04*sin(time*2.1);
        vec3 glow = localGlow(fragTexCoord, texel)*pulse;

        if (mode < 1.5)
        {
            color = grade(raw.rgb + glow*0.82, fragTexCoord);
        }
        else
        {
            vec3 pixelCore = pixelPalette(crisp.rgb, floor(gl_FragCoord.xy/block));
            color = grade(pixelCore + glow*0.62, fragTexCoord);
        }
    }

    vec4 tint = colDiffuse*fragColor;
    finalColor = vec4(clamp(color, 0.0, 1.0), raw.a)*tint;
}
