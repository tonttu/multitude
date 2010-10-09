
attribute vec2 pos;
attribute float in_size;

uniform vec2 viewsize;

varying float size;
varying vec2 vsiz;

void main()
{
  //Transform the vertex (ModelViewProj matrix)

  gl_Position = gl_ModelViewProjectionMatrix * vec4(pos, 0, 1);
  size = in_size;
  vsiz = viewsize; /*vec2(400.0, 400.0);*/
}
