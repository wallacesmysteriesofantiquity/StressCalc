#ifndef IMAGE_H
#define IMAGE_H

#include "./lodepng.h"
#include <cmath>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <queue>

//#define DEBUG_CTORS
#ifdef DEBUG_CTORS
	#define CTOR_OUT(x) std::cout << x << std::endl;
#else
	#define CTOR_OUT(x)
#endif

#ifdef _MSVC_LANG
	#define IMAGE_NO_EXCEPT noexcept
#else
	#define IMAGE_NO_EXCEPT
#endif



class Image {
public:
	static int qsort_compare_channel(const void* a, const void* b) {
		return (*static_cast<const uint8_t*>(a)) - (*static_cast<const uint8_t*>(b)); // Ascending order
	}
	static int qsort_compare_greyscale(const void* a, const void* b) {
		return GreyScale(*static_cast<const uint32_t*>(a)) - GreyScale(*static_cast<const uint32_t*>(b)); // Ascending order
	}

	static uint32_t Color(uint32_t r, uint32_t g, uint32_t b){ return r | (g << 8) | (b << 16) | 4278190080UL; }
	static uint32_t Color(uint32_t r, uint32_t g, uint32_t b, uint32_t a){ return r | (g << 8) | (b << 16) | (a << 24); }
	
	static uint32_t Color_f(float r, float g, float b){ return Color(fromFloat(r), fromFloat(g), fromFloat(b)); }
	static uint32_t Color_f(float r, float g, float b, float a){ return Color(fromFloat(r), fromFloat(g), fromFloat(b), fromFloat(a)); }
	
	static uint32_t Color_f(double r, double g, double b) { return Color(fromFloat(r), fromFloat(g), fromFloat(b)); }
	static uint32_t Color_f(double r, double g, double b, double a) { return Color(fromFloat(r), fromFloat(g), fromFloat(b), fromFloat(a)); }


	inline static uint8_t Red(uint32_t color){ return color & 255; }
	inline static uint8_t Green(uint32_t color){ return (color >> 8) & 255; }
	inline static uint8_t Blue(uint32_t color){ return (color >> 16) & 255; }
	inline static uint8_t Alpha(uint32_t color){ return static_cast<uint8_t>(color >> 24); }
	inline static float Red_f(uint32_t color){ return toFloat(Red(color)); }
	inline static float Green_f(uint32_t color){ return toFloat(Green(color)); }
	inline static float Blue_f(uint32_t color){ return toFloat(Blue(color)); }
	inline static float Alpha_f(uint32_t color){ return toFloat(Alpha(color)); }
	inline static float toFloat(uint8_t part){ return static_cast<float>(part) / 255.0f; }
	inline static uint8_t fromFloat(float part){ return static_cast<uint8_t>(255.0f * part); }
	inline static uint8_t fromFloat(double part) { return static_cast<uint8_t>(255.0 * part); }
	inline static uint8_t GreyScale(uint32_t color){
		uint32_t other = Red(color);
		other += Green(color);
		other += Blue(color);
		other /= 3;
		return static_cast<uint8_t>(other);
	}
	
	inline static uint32_t ColorBetween(uint32_t one, uint32_t two, float a){
		const float r = (Red_f(two) * a) + (Red_f(one) * (1.0f - a));
		const float g = (Green_f(two) * a) + (Green_f(one) * (1.0f - a));
		const float b = (Blue_f(two) * a) + (Blue_f(one) * (1.0f - a));
		return Color_f(r, g, b);
	}

	inline ~Image(){ 
		CTOR_OUT("Deleting " << static_cast<void*>(this));
	}
	inline Image(size_t width, size_t height, bool) : _width(static_cast<int>(width)), _widthTimes4(static_cast<int>(width << 2)), _height(static_cast<int>(height)), _image(width * height * 4){
		CTOR_OUT("Creating blank " << static_cast<void*>(this));
	}
	inline Image(int width, int height, bool) : _width(width), _widthTimes4(width * 4), _height(height), _image(static_cast<size_t>(width * height) * 4){
		CTOR_OUT("Creating blank " << static_cast<void*>(this));
	}
	inline Image(size_t width, size_t height, uint32_t fillcolor = 255) : _width(static_cast<int>(width)), _widthTimes4(static_cast<int>(width << 2)), _height(static_cast<int>(height)), _image(width * height * 4){
		CTOR_OUT("Creating fill " << static_cast<void*>(this));
		for(int x = 0; x < _width; ++x){
			for(int y = 0; y < _height; ++y){
				*(reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)])) = fillcolor;
			}
		}
	}
	inline Image(int width, int height, uint32_t fillcolor = 255) : _width(width), _widthTimes4(width * 4), _height(height), _image(static_cast<size_t>(width * height) * 4){
		CTOR_OUT("Creating fill " << static_cast<void*>(this));
		for(int x = 0; x < _width; ++x){
			for(int y = 0; y < _height; ++y){
				*(reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)])) = fillcolor;
			}
		}
	}
	inline Image(const char * filename) : _width(0), _widthTimes4(0), _height(0), _image(){ 
		CTOR_OUT("Creating cname " << static_cast<void*>(this));
		load(filename); 
	}
	inline Image(const std::string & filename) : _width(0), _widthTimes4(0), _height(0), _image(){ 
		CTOR_OUT("Creating sname " << static_cast<void*>(this));
		load(filename.c_str()); 
	}
	inline int width() const { return _width; }
	inline int height() const { return _height; }
	inline size_t pixels() const { return _image.size(); }
	inline std::vector<unsigned char>::size_type pixelIndex(int x, int y) const { return static_cast<std::vector<unsigned char>::size_type>(_widthTimes4 * y + (x << 2)); }


	inline void pset(int x, int y, uint32_t color){
		if (x < 0 || x >= _width) return;
		if (y < 0 || y >= _height) return;
		*(reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)])) = color;
	}

	inline void pset_unsafe(int x, int y, uint32_t color) {
		*(reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)])) = color;
	}
	
	void pset_blend(int x, int y, uint32_t color, float a = 1.0f){
		if (x < 0 || x >= _width) return;
		if (y < 0 || y >= _height) return;
		a *= Alpha_f(color);
		if (a <= 0.0f) return;
		const std::vector<unsigned char>::size_type ppp = pixelIndex(x, y);
		assert(ppp < _image.size());
		const uint32_t current = *(reinterpret_cast<uint32_t*>(&_image[ppp]));
		*(reinterpret_cast<uint32_t*>(&_image[ppp])) = ColorBetween(current, color, a);
	}

	void pset_blend_unsafe(int x, int y, uint32_t color, float a = 1.0f) {
		a *= Alpha_f(color);
		if (a <= 0.0f) return;
		const std::vector<unsigned char>::size_type ppp = pixelIndex(x, y);
		const uint32_t current = *(reinterpret_cast<uint32_t*>(&_image[ppp]));
		*(reinterpret_cast<uint32_t*>(&_image[ppp])) = ColorBetween(current, color, a);
	}
	
	inline uint32_t point(int x, int y) const {
		if (x < 0 || x >= _width) return 0;
		if (y < 0 || y >= _height) return 0;
		return *(reinterpret_cast<const uint32_t*>(&_image[pixelIndex(x, y)]));
	}
	
	inline const uint32_t & point_unsafe(int x, int y) const {
		assert(!(x < 0 || x >= _width));
		assert(!(y < 0 || y >= _height));
		return *(reinterpret_cast<const uint32_t*>(&_image[pixelIndex(x, y)]));
	}
	
	inline uint32_t & point_unsafe(int x, int y) {
		assert(!(x < 0 || x >= _width));
		assert(!(y < 0 || y >= _height));
		return *(reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)]));
	}
	
	Image get_width_and_height(int x, int y, int w, int h) const {
		if (x < 0){ w += x; x = 0; }
		if (y < 0){ h += y; y = 0; }
		if (w < 1) return Image(0,0);
		if (h < 1) return Image(0,0);
		int x2 = x + w;
		int y2 = y + h;
		if (x2 < 0) return Image(0,0);
		if (y2 < 0) return Image(0,0);
		if (x2 >= _width){ x2 = _width; w = x2 - x; }
		if (y2 >= _height){ y2 = _height; h = y2 - y; }
		Image res(w, h);
		const size_t bytesToCopy = static_cast<size_t>(w) << 2;
		uint32_t * dest = reinterpret_cast<uint32_t*>(&res._image[0]);
		const uint32_t * source = reinterpret_cast<const uint32_t*>(&_image[pixelIndex(x, y)]);
		for(; y < y2; ++y, dest += res._width, source += _width){ memcpy(dest, source, bytesToCopy); }
		return res;
	}
	
	Image get_x2_and_y2(int x, int y, int x2, int y2) const {
		if (x2 < x){ int tmp = x; x = x2; x2 = tmp; }
		if (y2 < y){ int tmp = y; y = y2; y2 = tmp; }
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x2 < 0) return Image(0,0);
		if (y2 < 0) return Image(0,0);
		if (x > _width) return Image(0,0);
		if (y > _height) return Image(0,0);
		if (x2 >= _width) x2 = _width - 1;
		if (y2 >= _height) y2 = _height - 1;
		const size_t xDelta = static_cast<size_t>(x2 - x);
		Image res(xDelta, static_cast<size_t>(y2 - y));
		const size_t bytesToCopy = xDelta * 4;
		uint32_t * dest = reinterpret_cast<uint32_t*>(&res._image[0]);
		const uint32_t * source = reinterpret_cast<const uint32_t*>(&_image[pixelIndex(x, y)]);
		for(; y < y2; ++y, dest += res._width, source += _width){ memcpy(dest, source, bytesToCopy); }
		return res;
	}
	
	
#define PUT_BOUNDARY_CHECK()\
		int startXImg = 0; int startYImg = 0;\
		int endXImg = img.width() - 1; int endYImg = img.height() - 1;\
		int startXThis = x; int startYThis = y;\
		const int endXThis = x + img.width() - 1; const int endYThis = y + img.height() - 1; \
		if (startXThis < 0){ startXImg -= startXThis; if (startXImg > endXImg) return; startXThis = 0; }\
		if (startYThis < 0){ startYImg -= startYThis; if (startYImg > endYImg) return; startYThis = 0; }\
		if (endXThis >= _width){ const int dx = endXThis - _width; endXImg -= dx; if (endXImg < startXImg) return; }\
		if (endYThis >= _height){ const int dy = endYThis - _height; endYImg -= dy; if (endYImg < startYImg) return; }	
	
	void put(const Image& img, int x, int y) {
		PUT_BOUNDARY_CHECK()

		const size_t bytesToCopy = static_cast<size_t>(endXImg - startXImg + 1) * 4;

		const uint32_t* source = reinterpret_cast<const uint32_t*>(&img._image[pixelIndex(startXImg, startYImg)]);
		uint32_t* dest = reinterpret_cast<uint32_t*>(&_image[pixelIndex(startXThis, startYThis)]);
		for (; startYThis <= endYThis; ++startYThis, source += img._width, dest += _width) { memcpy(dest, source, bytesToCopy); }
	}

	void put_mask(const Image & img, int x, int y){
		PUT_BOUNDARY_CHECK()
		
		int iyThis = startYThis;
		for(int iy = startYImg; iy <= endYImg; ++iy, ++iyThis){
			int ixThis = startXThis;
			for(int ix = startXImg; ix <= endXImg; ++ix, ++ixThis){
				const uint32_t color = img.point_unsafe(ix, iy);
				if (Alpha(color) == 255) pset_unsafe(ixThis, iyThis, color);
			}
		}
	}
	
	void put_blend(const Image & img, int x, int y, float a = 1.0f){
		PUT_BOUNDARY_CHECK()
		
		int iyThis = startYThis;
		for(int iy = startYImg; iy <= endYImg; ++iy, ++iyThis){
			int ixThis = startXThis;
			for(int ix = startXImg; ix <= endXImg; ++ix, ++ixThis){
				pset_blend_unsafe(ixThis, iyThis, img.point_unsafe(ix, iy), a);
			}
		}
	}
	
	void circle(int x, int y, int radius, uint32_t color){
		if (radius < 0) return;
		int yoff = 0;
		int err = 0;
		while (radius >= yoff){
			pset(x + radius, y + yoff, color);
			pset(x + yoff, y + radius, color);
			pset(x - yoff, y + radius, color);
			pset(x - radius, y + yoff, color);
			pset(x - radius, y - yoff, color);
			pset(x - yoff, y - radius, color);
			pset(x + yoff, y - radius, color);
			pset(x + radius, y - yoff, color);
			if (err <= 0){
				++yoff;
				err += (yoff << 1) + 1;
			}
			if (err > 0){
				--radius;
				err -= (radius << 1) + 1;
			}
		}
	}
	
	void circle_fill(int x, int y, int radius, uint32_t color){
		if (radius < 0) return;
		int yoff = 0;
		int err = 0;
		while (radius >= yoff){
			hline(y + radius, x - yoff, x + yoff, color);
			hline(y + yoff, x - radius, x + radius, color);
			hline(y - yoff, x - radius, x + radius, color);
			hline(y - radius, x - yoff, x + yoff, color);
			
			if (err <= 0){
				++yoff;
				err += (yoff << 1) + 1;
			}
			if (err > 0){
				--radius;
				err -= (radius << 1) + 1;
			}
		}
	}
	
	inline void circle_fill_w_outline(int x, int y, int radius, uint32_t fillcolor, uint32_t outlinecolor){
		circle_fill(x, y, radius, fillcolor);
		circle(x, y, radius, outlinecolor);
	}
	
	inline void vline(int x, int y, int y2, uint32_t color){
		if (x < 0 || x >= _width) return;
		if (y < 0) y = 0;
		if (y2 >= _height) y2 = _height - 1;
		uint32_t* iterato = reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)]);
		const uint32_t* en = iterato + (y2 - y + 1) * _width;
		for (; iterato < en; iterato += _width) *(iterato) = color;
	}
	inline void hline(int y, int x, int x2, uint32_t color){
		if (y < 0 || y >= _height) return;
		if (x < 0) x = 0;
		if (x2 >= _width) x2 = _width - 1;
		uint32_t * iterato = reinterpret_cast<uint32_t*>(&_image[pixelIndex(x, y)]);
		const uint32_t* en = iterato + (x2 - x + 1);
		for (; iterato < en; ++iterato) *(iterato) = color;
	}
	
	void line(int x, int y, int x2, int y2, uint32_t color){
		const int dx = abs(x2 - x);
		const int sx = x < x2 ? 1 : -1;
		const int dy = -abs(y2 - y);
		const int sy = y < y2 ? 1 : -1;
		int error = dx + dy;
		int e2;
		while(true){
			pset(x, y, color);
			if ((x == x2) && (y == y2)) return;
			e2 = error << 1;
			if (e2 >= dy){
				if (x == x2) return;
				error += dy;
				x += sx;
			}
			if (e2 <= dx){
				if (y == y2) return;
				error += dx;
				y += sy;
			}
		}
	}
	
	void line_gradient(int x, int y, int x2, int y2, uint32_t color1, uint32_t color2){
		const int dx = abs(x2 - x);
		const int sx = x < x2 ? 1 : -1;
		const int dy = -abs(y2 - y);
		const int sy = y < y2 ? 1 : -1;
		int error = dx + dy;
		int e2;
		float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));
		while(true){
			const int ddx = abs(x2 - x);
			const int ddy = -abs(y2 - y);
			float dist2 = sqrtf(static_cast<float>(ddx * ddx + ddy * ddy));
			
			pset_unsafe(x, y, ColorBetween(color1, color2, 1.0f - (dist2/ dist)));
			if ((x == x2) && (y == y2)) return;
			e2 = error << 1;
			if (e2 >= dy){
				if (x == x2) return;
				error += dy;
				x += sx;
			}
			if (e2 <= dx){
				if (y == y2) return;
				error += dx;
				y += sy;
			}
		}
	}
	
	void line_blend(int x, int y, int x2, int y2, uint32_t color, float a = 1.0f){
		const int dx = abs(x2 - x);
		const int sx = x < x2 ? 1 : -1;
		const int dy = -abs(y2 - y);
		const int sy = y < y2 ? 1 : -1;
		int error = dx + dy;
		int e2;
		while(true){
			pset_blend_unsafe(x, y, color, a);
			if ((x == x2) && (y == y2)) return;
			e2 = error << 1;
			if (e2 >= dy){
				if (x == x2) return;
				error += dy;
				x += sx;
			}
			if (e2 <= dx){
				if (y == y2) return;
				error += dx;
				y += sy;
			}
		}
	}
	
	void rect_x2_and_y2(int x, int y, int x2, int y2, uint32_t color){
		if (x2 < x){ int tmp = x; x = x2; x2 = tmp; }
		if (y2 < y){ int tmp = y; y = y2; y2 = tmp; }
		const int xPlus1 = x + 1;
		const int x2Minus1 = x2 - 1;
		vline(x, y, y2, color);
		vline(x2, y, y2, color);
		hline(y, xPlus1, x2Minus1, color);
		hline(y2, xPlus1, x2Minus1, color);
	}
	
	void rect_width_and_height(int x, int y, int w, int h, uint32_t color){
		if (w < 1) return;
		if (h < 1) return;
		const int x2 = x + w;
		const int y2 = y + h;
		const int xPlus1 = x + 1;
		const int x2Minus1 = x2 - 1;
		vline(x, y, y2, color);
		vline(x2, y, y2, color);
		hline(y, xPlus1, x2Minus1, color);
		hline(y2, xPlus1, x2Minus1, color);
	}
	
	inline void rect_fill_x2_and_y2(int x, int y, int x2, int y2, uint32_t color){
		if (x2 < x){ int tmp = x; x = x2; x2 = tmp; }
		if (y2 < y){ int tmp = y; y = y2; y2 = tmp; }
		for(;y <= y2; ++y) hline(y, x, x2, color);
	}
	
	void rect_fill_width_and_height(int x, int y, int w, int h, uint32_t color){
		if (w < 1) return;
		if (h < 1) return;
		const int x2 = x + w;
		const int y2 = y + h;
		for (; y <= y2; ++y) hline(y, x, x2, color);
	}
	
	inline void rect_fill_w_outline_x2_and_y2(int x, int y, int x2, int y2, uint32_t fillcolor, uint32_t outlinecolor){
		rect_x2_and_y2(x, y, x2, y2, outlinecolor);
		rect_fill_x2_and_y2(x + 1, y + 1, x2 - 1, y2 - 1, fillcolor);
	}
	
	inline void rect_fill_w_outline_width_and_height(int x, int y, int w, int h, uint32_t fillcolor, uint32_t outlinecolor){
		rect_width_and_height(x, y, w, h, outlinecolor);
		rect_fill_width_and_height(x + 1, y + 1, w - 2, h - 2, fillcolor);
	}
	
	
	inline bool save(const std::string & filename) const { return save(filename.c_str()); }
	bool save(const char * filename) const {
		if (_width <= 0 || _height <= 0){
			std::cerr << "Trying to save with dimentions of " << _width << " x " << _height << std::endl;
			return false;
		}
		unsigned int error = lodepng::encode(filename, _image, static_cast<unsigned int>(_width), static_cast<unsigned int>(_height));
		if (error){
			std::cerr << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
			std::cerr << filename << std::endl;
			return false;
		}
		return true;
	}
	
	inline bool load(const std::string & filename) { return load(filename.c_str()); }
	bool load(const char * filename){
		unsigned int w, h;
		unsigned int error = lodepng::decode(_image, w, h, filename);
		if (error){
			std::cerr << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			std::cerr << filename << std::endl;
			return false;
		}
		_width = static_cast<int>(w);
		_widthTimes4 = _width << 2;
		_height = static_cast<int>(h);
		return true;
	}
	
	Image vflip() const {
		Image res(_width, _height, true);

		for (int y = 0; y < _height; ++y) {
			const uint32_t* source = reinterpret_cast<const uint32_t*>(&_image[static_cast<size_t>(_widthTimes4 * y)]);
			uint32_t* dest = reinterpret_cast<uint32_t*>(&res._image[static_cast<size_t>(_widthTimes4 * (_height - 1 - y))]);
			memcpy(dest, source, static_cast<size_t>(_widthTimes4));
		}
		return res;
	}
	
	Image hflip() const {
		Image res(_width, _height, true);
		for(int x = 0; x < _width; ++x){
			for(int y = 0; y < _height; ++y){
				res.pset_unsafe(x, y, point_unsafe(_width - 1 - x, y));
			}
		}
		return res;
	}
	
	Image rotateCW() const {
		Image res(_height, _width, true);
		for(int x = 0; x < _width; ++x){
			for(int y = 0; y < _height; ++y){
				res.pset_unsafe(y, x, point_unsafe(x, y));
			}
		}
		return res;
	}
	
	Image rotateCCW() const {
		Image res(_height, _width, true);
		for(int x = 0; x < _height; ++x){
			for(int y = 0; y < _width; ++y){
				res.pset_unsafe(x, y, point_unsafe(_width - 1 - y, _height - 1 - x));
			}
		}
		return res;
	}
	
	inline uint8_t * data(){ return &_image[0]; }
    inline const uint8_t * data() const { return &_image[0]; }


    
	inline Image(const Image &o) IMAGE_NO_EXCEPT : _width(o._width), _widthTimes4(o._widthTimes4), _height(o._height), _image() {
		_image.reserve(o._image.size());
		_image.resize(o._image.size());
		memcpy(_image.data(), o._image.data(), _image.size());
	}
	inline Image(Image&&o) IMAGE_NO_EXCEPT : _width(o._width), _widthTimes4(o._widthTimes4), _height(o._height), _image() {
		_image.reserve(o._image.size());
		_image.resize(o._image.size());
		memcpy(_image.data(), o._image.data(), _image.size());
	}
	Image& operator=(const Image& o){
		_width = o._width;
		_widthTimes4 = o._widthTimes4;
		_height = o._height;
		_image.reserve(o._image.size());
		_image.resize(o._image.size());
		memcpy(_image.data(), o._image.data(), _image.size());
		return *this;
	}
	Image& operator=(Image&&o) IMAGE_NO_EXCEPT {
		_width = o._width;
		_widthTimes4 = o._widthTimes4;
		_height = o._height;
		_image = o._image;
		_image.reserve(o._image.size());
		_image.resize(o._image.size());
		memcpy(_image.data(), o._image.data(), _image.size());
		return *this;
	}

	bool operator ==(const Image& other) {
		if (_width != other._width) return false;
		if (_height != other._height) return false;
		if (_image.size() != other._image.size()) return false;
		return memcmp(_image.data(), other._image.data(), _image.size()) == 0;
	}

	bool operator !=(const Image& other) {
		if (_width != other._width) return true;
		if (_height != other._height) return true;
		if (_image.size() != other._image.size()) return true;
		return memcmp(_image.data(), other._image.data(), _image.size()) != 0;
	}

	Image resizeMaintainAspect(int w2, int h2) const {
		// Calculate resize ratios for resizing 
		float ratioW = static_cast<float>(w2) / static_cast<float>(width());
		float ratioH = static_cast<float>(h2) / static_cast<float>(height());

		// smaller ratio will ensure that the image fits in the view
		float ratio = ratioW < ratioH ? ratioW : ratioH;

		w2 = static_cast<int>(static_cast<float>(width()) * ratio);
		h2 = static_cast<int>(static_cast<float>(height()) * ratio);
		return resize(w2, h2);
	}
	
	Image resize(int w2, int h2) const {
		Image temp(w2, h2, true);
		int x, y;
		float x_ratio = (static_cast<float>(_width - 1))/static_cast<float>(w2);
		float y_ratio = (static_cast<float>(_height - 1))/static_cast<float>(h2);
		float x_diff, y_diff, one_minus_xdiff, one_minus_ydiff, x_diff_time_y_diff, x_diff_times_one_minus_ydiff, y_diff_times_one_minus_xdiff, one_minus_ydiff_time_one_minus_xdiff;
		float blue, red, green, alpha;
		for (int i=0;i<h2;i++) {
			for (int j=0;j<w2;j++) {
				x = static_cast<int>(x_ratio * static_cast<float>(j)) ;
				y = static_cast<int>(y_ratio * static_cast<float>(i)) ;
				x_diff = (x_ratio * static_cast<float>(j)) - static_cast<float>(x);
				y_diff = (y_ratio * static_cast<float>(i)) - static_cast<float>(y);
				x_diff_time_y_diff = x_diff * y_diff;
				one_minus_xdiff = 1.0f - x_diff;
				one_minus_ydiff = 1.0f - y_diff;
				one_minus_ydiff_time_one_minus_xdiff = one_minus_xdiff * one_minus_ydiff;
				x_diff_times_one_minus_ydiff = x_diff * one_minus_ydiff;
				y_diff_times_one_minus_xdiff = y_diff * one_minus_xdiff;
				uint32_t a = point_unsafe(x, y);
				uint32_t b = point_unsafe(x + 1, y);
				uint32_t c = point_unsafe(x, y + 1);
				uint32_t d = point_unsafe(x + 1, y + 1);

				// Yr = Ab(1-w)(1-h) + Bb(w)(1-h) + Cb(h)(1-w) + Db(wh)
				red = static_cast<float>(a&0xff)       *one_minus_ydiff_time_one_minus_xdiff + static_cast<float>(b&0xff)      * x_diff_times_one_minus_ydiff + static_cast<float>(c&0xff)      * y_diff_times_one_minus_xdiff + static_cast<float>(d&0xff)       * x_diff_time_y_diff;

				// Yg = Ag(1-w)(1-h) + Bg(w)(1-h) + Cg(h)(1-w) + Dg(wh)
				green = static_cast<float>((a>>8)&0xff)*one_minus_ydiff_time_one_minus_xdiff + static_cast<float>((b>>8)&0xff) * x_diff_times_one_minus_ydiff + static_cast<float>((c>>8)&0xff) * y_diff_times_one_minus_xdiff + static_cast<float>((d>>8)&0xff)  * x_diff_time_y_diff;

				// Yb = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
				blue = static_cast<float>((a>>16)&0xff)*one_minus_ydiff_time_one_minus_xdiff + static_cast<float>((b>>16)&0xff)* x_diff_times_one_minus_ydiff + static_cast<float>((c>>16)&0xff)* y_diff_times_one_minus_xdiff + static_cast<float>((d>>16)&0xff) * x_diff_time_y_diff;

				// Yb = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
				alpha = static_cast<float>((a >> 24) & 0xff) * one_minus_ydiff_time_one_minus_xdiff + static_cast<float>((b >> 24) & 0xff) * x_diff_times_one_minus_ydiff + static_cast<float>((c >> 24) & 0xff) * y_diff_times_one_minus_xdiff + static_cast<float>((d >> 24) & 0xff) * x_diff_time_y_diff;

				temp.pset_unsafe(j, i, Color(static_cast<uint32_t>(red), static_cast<uint32_t>(green), static_cast<uint32_t>(blue), static_cast<uint32_t>(alpha)));
			}
		}
		return temp;
	}
  
	void replaceColor(uint32_t find, uint32_t replace){
		uint32_t * data32 = reinterpret_cast<uint32_t*>(data());
		uint32_t * end = data32 + (pixels() / 4);
		for(; data32 < end; ++data32){
			if (*data32 == find) *data32 = replace;
		}
	}
	
	Image noiseRemove_Median_PerChannel(int radius = 1){
		assert(radius > 0);
		const size_t p1 = static_cast<size_t>(radius) * 2 + 1;
		uint8_t * window = static_cast<uint8_t*>(malloc(p1 * p1 * sizeof(uint8_t)));
		Image secondImage(width(), height(), true);
		for(int row = radius; row < width() - radius; ++row){
			for(int col = radius; col < height() - radius; ++col){
				size_t index = 0;
				for(int offsetX = -radius; offsetX <= radius; ++offsetX){
					for(int offsetY = -radius; offsetY <= radius; ++offsetY){
						window[index++] = Red(point_unsafe(row + offsetX, col + offsetY));
					}
				}
				assert(index > 0);
				std::qsort(window, index, sizeof(uint8_t), qsort_compare_channel); 
				uint8_t red = window[index / 2];
				
				index = 0;
				for(int offsetX = -1; offsetX <= 1; ++offsetX){
					for(int offsetY = -1; offsetY <= 1; ++offsetY){
						window[index++] = Green(point_unsafe(row + offsetX, col + offsetY));
					}
				}
				assert(index > 0);
				std::qsort(window, index, sizeof(uint8_t), qsort_compare_channel); 
				uint8_t green = window[index / 2];
				
				index = 0;
				for(int offsetX = -1; offsetX <= 1; ++offsetX){
					for(int offsetY = -1; offsetY <= 1; ++offsetY){
						window[index++] = Blue(point_unsafe(row + offsetX, col + offsetY));
					}
				}
				assert(index > 0);
				std::qsort(window, index, sizeof(uint8_t), qsort_compare_channel); 
				uint8_t blue = window[index / 2];
				
				secondImage.pset_unsafe(row, col, Color(red, green, blue));
			}
		}
		free(window);
		return secondImage;
	}
	
	Image noiseRemove_Median_Greyscale(int radius = 1){
		assert(radius > 0);
		const size_t p1 = static_cast<size_t>(radius) * 2 + 1;
		uint32_t * window = static_cast<uint32_t*>(malloc(p1 * p1 * sizeof(uint32_t)));
		Image secondImage(width(), height(), true);
		for(int row = radius; row < width() - radius; ++row){
			for(int col = radius; col < height() - radius; ++col){
				size_t index = 0;
				for(int offsetX = -radius; offsetX <= radius; ++offsetX){
					for(int offsetY = -radius; offsetY <= radius; ++offsetY){
						window[index++] = point_unsafe(row + offsetX, col + offsetY);
					}
				}
				assert(index > 0);
				std::qsort(window, index, sizeof(uint32_t), qsort_compare_greyscale); 
				uint32_t color = window[index / 2];				
				secondImage.pset_unsafe(row, col, color);
			}
		}
		free(window);
		return secondImage;
	}
	
	Image toGreyscale(){
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t gray = GreyScale(point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(gray, gray, gray));
			}
		}
		return secondImage;
	}
	
	Image swapRedAndBlue(){
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint32_t c = point_unsafe(row, col);
				uint32_t r = Red(c);
				uint32_t g = Green(c);
				uint32_t b = Blue(c);
				uint32_t a = Alpha(c);
				secondImage.pset_unsafe(row, col, Color(b, g, r, a));
			}
		}
		return secondImage;
	}
	
	Image redChannelToGreyscale(){
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t gray = Red(point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(gray, gray, gray));
			}
		}
		return secondImage;
	}
	
	Image greenChannelToGreyscale(){
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t gray = Red(point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(gray, gray, gray));
			}
		}
		return secondImage;
	}
	
	Image blueChannelToGreyscale(){
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t gray = Red(point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(gray, gray, gray));
			}
		}
		return secondImage;
	}
	



	Image edgeDetect_CannyFilter_Channels(double lowerThreshold, double higherThreshold) {
		Image edgeRed = redChannelToGreyscale().edgeDetect_CannyFilter_Greyscale(lowerThreshold, higherThreshold);
		Image edgeGreen = greenChannelToGreyscale().edgeDetect_CannyFilter_Greyscale(lowerThreshold, higherThreshold);
		Image edgeBlue = blueChannelToGreyscale().edgeDetect_CannyFilter_Greyscale(lowerThreshold, higherThreshold);
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t red = Red(edgeRed.point_unsafe(row, col));
				uint8_t green = Red(edgeGreen.point_unsafe(row, col));
				uint8_t blue = Red(edgeBlue.point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(red, green, blue));
			}
		}
		return secondImage;
	}


	Image edgeDetect_CannyFilter_Greyscale(double lowerThreshold, double higherThreshold) {
		Image pixelsCanny(width(), height(), true);
		int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
		int gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
		double * G = static_cast<double*>(malloc(static_cast<size_t>(pixels() * sizeof(double))));
		std::vector<int> theta(pixels());
		double largestG = 0;
		// perform canny edge detection on everything but the edges
		for (int i = 1; i < height() - 1; i++) {
			for (int j = 1; j < width() - 1; j++) {
				// find gx and gy for each pixel
				double gxValue = 0;
				double gyValue = 0;
				for (int x = -1; x <= 1; x++) {
					for (int y = -1; y <= 1; y++) {
						gxValue = gxValue + (gx[1 - x][1 - y] * static_cast<double>(GreyScale(point_unsafe(j + x, i + y))));
						gyValue = gyValue + (gy[1 - x][1 - y] * static_cast<double>(GreyScale(point_unsafe(j + x, i + y))));
					}
				}

				// calculate G and theta
				G[i * width() + j] = std::sqrt(std::pow(gxValue, 2) + std::pow(gyValue, 2));
				double atanResult = atan2(gyValue, gxValue) * 180.0 / 3.14159265;
				theta[static_cast<size_t>(i * width() + j)] = static_cast<int>(180.0 + atanResult);

				if (G[static_cast<size_t>(i * width() + j)] > largestG) { largestG = G[i * width() + j]; }

				// setting the edges
				if (i == 1) {
					G[static_cast<size_t>(i * width() + j - 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>(i * width() + j - 1)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (j == 1) {
					G[static_cast<size_t>((i - 1) * width() + j)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i - 1) * width() + j)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (i == height() - 1) {
					G[static_cast<size_t>(i * width() + j + 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>(i * width() + j + 1)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (j == width() - 1) {
					G[static_cast<size_t>((i + 1) * width() + j)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i + 1) * width() + j)] = theta[static_cast<size_t>(i * width() + j)];
				}

				// setting the corners
				if (i == 1 && j == 1) {
					G[static_cast<size_t>((i - 1) * width() + j - 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i - 1) * width() + j - 1)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (i == 1 && j == width() - 1) {
					G[static_cast<size_t>((i - 1) * width() + j + 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i - 1) * width() + j + 1)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (i == height() - 1 && j == 1) {
					G[static_cast<size_t>((i + 1) * width() + j - 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i + 1) * width() + j - 1)] = theta[static_cast<size_t>(i * width() + j)];
				} else if (i == height() - 1 && j == width() - 1) {
					G[static_cast<size_t>((i + 1) * width() + j + 1)] = G[static_cast<size_t>(i * width() + j)];
					theta[static_cast<size_t>((i + 1) * width() + j + 1)] = theta[static_cast<size_t>(i * width() + j)];
				}

				// round to the nearest 45 degrees
				theta[static_cast<size_t>(i * width() + j)] = static_cast<int>(round(theta[static_cast<size_t>(i * width() + j)] / 45) * 45);
			}
		}
		
		// non-maximum suppression
		for (int i = 1; i < height() - 1; i++) {
			for (int j = 1; j < width() - 1; j++) {
				if (theta[static_cast<size_t>(i * width() + j)] == 0 || theta[static_cast<size_t>(i * width() + j)] == 180) {
					if (G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>(i * width() + j - 1)] || G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>(i * width() + j + 1)]) {
						G[static_cast<size_t>(i * width() + j)] = 0;
					}
				} else if (theta[static_cast<size_t>(i * width() + j)] == 45 || theta[static_cast<size_t>(i * width() + j)] == 225) {
					if (G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i + 1) * width() + j + 1)] || G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i - 1) * width() + j - 1)]) {
						G[static_cast<size_t>(i * width() + j)] = 0;
					}
				} else if (theta[static_cast<size_t>(i * width() + j)] == 90 || theta[static_cast<size_t>(i * width() + j)] == 270) {
					if (G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i + 1) * width() + j)] || G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i - 1) * width() + j)]) {
						G[static_cast<size_t>(i * width() + j)] = 0;
					}
				} else {
					if (G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i + 1) * width() + j - 1)] || G[static_cast<size_t>(i * width() + j)] < G[static_cast<size_t>((i - 1) * width() + j + 1)]) {
						G[static_cast<size_t>(i * width() + j)] = 0;
					}
				}

				uint8_t c = static_cast<uint8_t>(G[static_cast<size_t>(i * width() + j)] * (255.0 / largestG));
				pixelsCanny.pset_unsafe(j, i, Color(c, c, c));
			}
		}

		// double threshold
		bool changes;
		do {
			changes = false;
			for (int i = 1; i < height() - 1; i++) {
				for (int j = 1; j < width() - 1; j++) {
					if (G[i * width() + j] < (lowerThreshold * largestG)) {
						G[i * width() + j] = 0;
					} else if (G[i * width() + j] >= (higherThreshold * largestG)) {
						continue;
					} else if (G[i * width() + j] < (higherThreshold * largestG)) {
						G[i * width() + j] = 0;
						for (int x = -1; x <= 1; x++) {
							bool breakNestedLoop = false;
							for (int y = -1; y <= 1; y++) {
								if (x == 0 && y == 0) { continue; }
								if (G[(i + x) * width() + (j + y)] >= (higherThreshold * largestG)) {
									G[i * width() + j] = (higherThreshold * largestG);
									changes = true;
									breakNestedLoop = true;
									break;
								}
							}
							if (breakNestedLoop) { break; }
						}
					}
					uint8_t c = static_cast<uint8_t>(G[static_cast<size_t>(i * width() + j)] * (255.0 / largestG));
					pixelsCanny.pset_unsafe(j, i, Color(c, c, c));
				}
			}
		} while (changes);
		
		free(G);
		return pixelsCanny;
	}



	Image edgeDetect_Sobel_Channels() {
		Image edgeRed = redChannelToGreyscale().edgeDetect_Sobel_Greyscale();
		Image edgeGreen = greenChannelToGreyscale().edgeDetect_Sobel_Greyscale();
		Image edgeBlue = blueChannelToGreyscale().edgeDetect_Sobel_Greyscale();
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t red = Red(edgeRed.point_unsafe(row, col));
				uint8_t green = Red(edgeGreen.point_unsafe(row, col));
				uint8_t blue = Red(edgeBlue.point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(red, green, blue));
			}
		}
		return secondImage;
	}

	Image edgeDetect_Sobel_Greyscale(){
		Image pixelsCanny(width(), height());
		int i, j = 0;
		uint32_t * p1 = reinterpret_cast<uint32_t*>(data()); 
		uint32_t * p2 = p1+width(); 
		uint32_t * p3 = p2+width();

		for( j = 0; j < height()-2; ++j ){
			for( i = 0; i < width()-2; ++i ){				
				uint8_t c = static_cast<uint8_t>((abs((GreyScale(p1[0]) + 2*GreyScale(p1[1]) + GreyScale(p1[2])) - (GreyScale(p3[0]) + 2*GreyScale(p3[1])+GreyScale(p3[2]))) + abs((GreyScale(p1[2]) + 2*GreyScale(p2[2]) + GreyScale(p3[2])) - (GreyScale(p1[0]) + 2*GreyScale(p2[0]) + GreyScale(p3[0]))) )/ 6);
				pixelsCanny.pset(i + 1, j + 1, Image::Color(c, c, c));
				++p1; ++p2; ++p3;
			}
			p1 += 2; p2 += 2; p3 +=2;
		}
		return pixelsCanny;
	}
	
	Image edgeDetect_GaussianDiff(float r1, float r2){ 
		Image secondImage(width(), height(), true);
		Image blur1 = gaussianBlur(r1);
		Image blur2 = gaussianBlur(r2);
		float minR = 999999;
		float minG = 999999;
		float minB = 999999;
		float maxR = -999999;
		float maxG = -999999;
		float maxB = -999999;
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = Red_f(blur1.point_unsafe(row, col)) - Red_f(blur2.point_unsafe(row, col));
				if (red < minR) minR = red;
				if (red > maxR) maxR = red;
				float green = Green_f(blur1.point_unsafe(row, col)) - Green_f(blur2.point_unsafe(row, col));
				if (green < minG) minG = green;
				if (green > maxG) maxG = green;
				float blue = Blue_f(blur1.point_unsafe(row, col)) - Blue_f(blur2.point_unsafe(row, col));
				if (blue < minB) minB = blue;
				if (blue > maxB) maxB = blue;
			}
		}
		
		
				
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = Red_f(blur1.point_unsafe(row, col)) - Red_f(blur2.point_unsafe(row, col));
				red += minR;
				red /= (maxR - minR);
				
				float green = Green_f(blur1.point_unsafe(row, col)) - Green_f(blur2.point_unsafe(row, col));
				green += minG;
				green /= (maxG - minG);
				
				float blue = Blue_f(blur1.point_unsafe(row, col)) - Blue_f(blur2.point_unsafe(row, col));
				blue += minB;
				blue /= (maxB - minB);
				
				secondImage.pset_unsafe(row, col, Color_f(red, green, blue));
			}
		}
		
		return secondImage;
	}
	
	Image sqrt_Channels(){
		Image secondImage(width(), height(), true);
		float minR = 999999;
		float minG = 999999;
		float minB = 999999;
		float maxR = -999999;
		float maxG = -999999;
		float maxB = -999999;
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = std::sqrt(Red_f(point_unsafe(row, col)));
				if (red < minR) minR = red;
				if (red > maxR) maxR = red;
				float green = std::sqrt(Green_f(point_unsafe(row, col)));
				if (green < minG) minG = green;
				if (green > maxG) maxG = green;
				float blue = std::sqrt(Blue_f(point_unsafe(row, col)));
				if (blue < minB) minB = blue;
				if (blue > maxB) maxB = blue;
			}
		}
		
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = std::sqrt(Red_f(point_unsafe(row, col)));
				red += minR;
				red /= (maxR - minR);
				
				float green = std::sqrt(Green_f(point_unsafe(row, col)));
				green += minG;
				green /= (maxG - minG);
				
				float blue = std::sqrt(Blue_f(point_unsafe(row, col)));
				blue += minB;
				blue /= (maxB - minB);
				
				secondImage.pset_unsafe(row, col, Color_f(red, green, blue));
			}
		}
		
		return secondImage;
	}
	
	Image sqrt_Greyscale(){
		Image secondImage(toGreyscale());
		float minR = 999999;
		float maxR = -999999;
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = std::sqrt(Red_f(secondImage.point_unsafe(row, col)));
				if (red < minR) minR = red;
				if (red > maxR) maxR = red;
			}
		}
		
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				float red = std::sqrt(Red_f(point_unsafe(row, col)));
				red += minR;
				red /= (maxR - minR);
				
				secondImage.pset_unsafe(row, col, Color_f(red, red, red));
			}
		}
		
		return secondImage;
	}



	Image inverse() {
		Image secondImage(width(), height(), true);
		for(int row = 0; row < width(); ++row){
			for(int col = 0; col < height(); ++col){
				uint8_t red = 255 - Red(point_unsafe(row, col));
				uint8_t green = 255 - Green(point_unsafe(row, col));
				uint8_t blue = 255 - Blue(point_unsafe(row, col));
				secondImage.pset_unsafe(row, col, Color(red, green, blue));
			}
		}
		return secondImage;
	}




	Image gaussianBlur(float radius){	//nombre d'octets par pixel	
		Image secondImage(width(), height(), true);
		unsigned char* temp=new unsigned char[_widthTimes4 *_height];
		float sigma2=radius*radius;	
		int size=5;
		//good approximation of filter	
		float pixel[4];	
		float sum;		
		//blurs x components	
		for(int y=0;y<_height;y++)	{
			for(int x=0;x<_width;x++)		{
				//process a pixel
				sum=0;			
				pixel[0]=0;			
				pixel[1]=0;			
				pixel[2]=0;			
				pixel[3]=0;			
				//accumulate colors			
				for(int i=std::max(0,x-size);i<=std::min(_width-1,x+size);i++)			{
					float factor= static_cast<float>(exp(static_cast<double>(-(i - x) * (i - x)) / (2.0 * sigma2)));
					sum+=factor;				
					for(int c=0;c<4;c++)				{					
						pixel[c]+=factor*static_cast<float>(_image[static_cast<size_t>((i+y*_width)*4+c)]);
					};			
				};			
				//copy a pixel			
				for(int c=0;c<4;c++)			{				
					temp[(x+y*_width)*4+c]=static_cast<unsigned char>(pixel[c]/sum);
				};		
			};	
		};	
		//blurs x components	
		for(int y=0;y<_height;y++)	{		
			for(int x=0;x<_width;x++)		{			
				//process a pixel			
				sum=0;			
				pixel[0]=0;			
				pixel[1]=0;			
				pixel[2]=0;			
				pixel[3]=0;			
				//accumulate colors			
				for(int i=std::max(0,y-size);i<=std::min(_height-1,y+size);i++)			{
					float factor = static_cast<float>(exp(static_cast<double>(-(i - y) * (i - y)) / (2.0 * sigma2)));
					sum+=factor;				
					for(int c=0;c<4;c++)				{					
						pixel[c]+=factor*static_cast<float>(temp[static_cast<size_t>((x+i*_width)*4+c)]);
					};			
				};			
				//copy a pixel			
				for(int c=0;c<4;c++)			{				
					secondImage._image[static_cast<size_t>((x+y*_width)*4+c)]=static_cast<unsigned char>(pixel[c]/sum);
				};		
			};	
		};	
		delete[] temp;
		return secondImage;
	};

	void flood_fill_recursive(int x, int y, uint32_t color){
		if (x < 0 || x >= width() || y < 0 || y >= height()) return;
		if (point_unsafe(x, y) == color) return;
		pset(x, y, color);
 
		flood_fill(x+1, y, color);
		flood_fill(x-1, y, color);
		flood_fill(x, y+1, color);
		flood_fill(x, y-1, color);
	}
	
	void flood_fill(int x, int y, uint32_t color){
		uint32_t oldColor = point(x, y);
		if (oldColor == color) return;

		std::queue<std::pair<int, int> > q;
		q.push({x, y});

		while (!q.empty()) {
			int xx = q.front().first;
			int yy = q.front().second;
			q.pop();

			if (xx < 0 || xx >= width() || yy < 0 || yy >= height() || point(xx, yy) == color) {
				continue;
			}

			pset(xx, yy, color);

			q.push({xx + 1, yy});
			q.push({xx - 1, yy});
			q.push({xx, yy + 1});
			q.push({xx, yy - 1});
		}
	}

	

private:
	friend class Font;
	inline Image() : _width(0), _widthTimes4(0), _height(0), _image(){
		CTOR_OUT("Creating private blank " << static_cast<void*>(this));
	}
	
	

	int _width;
	int _widthTimes4;
	int _height;
	std::vector<uint8_t> _image;
};

#endif
