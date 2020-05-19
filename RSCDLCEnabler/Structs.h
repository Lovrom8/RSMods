#pragma once
enum string_state {
	normal = 0x368,
	disabled = 0x380,
	highlight = 0x398,
	meme2 = 0x428,
	meme = 0x440,
	meme3 = 0x458
};

const float M_PI = 3.14159265359;
struct Color {
	float r;
	float g;
	float b;

	void setH(float H) {
		float U = cos(H*M_PI / 180);
		float W = sin(H*M_PI / 180);

		float _r = 1.f;
		float _g = 0.f;
		float _b = 0.f;

		r = (.299 + .701*U + .168*W)*_r
			+ (.587 - .587*U + .330*W)*_g
			+ (.114 - .114*U - .497*W)*_b;

		g = (.299 - .299*U - .328*W)*_r
			+ (.587 + .413*U + .035*W)*_g
			+ (.114 - .114*U + .292*W)*_b;

		b = (.299 - .3*U + 1.25*W)*_r
			+ (.587 - .588*U - 1.05*W)*_g
			+ (.114 + .886*U - .203*W)*_b;
	}
};
