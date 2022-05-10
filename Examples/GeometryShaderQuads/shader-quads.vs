
attribute vec2 pos;
attribute float size;
attribute float alpha;

varying float gs_size;
varying float gs_alpha;

void main()
{

  gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0, 1);
  gs_size = size;
  gs_alpha = alpha;
}
