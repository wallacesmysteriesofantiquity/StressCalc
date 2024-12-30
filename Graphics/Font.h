#ifndef FONT_H
#define FONT_H

#include "./Image.h"

class Font {
public:
	Font(const std::string & filename, int characterWidth = -1, int characterHeight = -1, bool vflip = false, float scalar = 1.0f){
		Image loader(filename);
		int width = loader.width() / 16;
		int height = loader.height() / 16;
		
		if (characterWidth < 0) characterWidth = width;
		if (characterHeight < 0) characterHeight = height;
		
		int i = 0;
		for(int y = 0; y < 16; ++y){
			for(int x = 0; x < 16; ++x){
				characters[i] = loader.get_width_and_height(x * width, y * height, characterWidth, characterHeight);
				characters[i].replaceColor(Image::Color(255, 0, 255), Image::Color(0,0,0,0));
				autoCropX(characters[i]);
				if (vflip) characters[i] = characters[i].vflip();
				if (scalar > 1.01f || scalar < 0.99f) characters[i] = characters[i].resize(static_cast<int>(static_cast<float>(characters[i].width()) * scalar), static_cast<int>(static_cast<float>(characters[i].height()) * scalar));
				++i;
			}
		}
	}
	
	inline int height() const { return characters[0].height(); }
	
	void write(const std::string & str, Image & img, int x, int y){
		int realX = x;
		for(auto & c : str){
			if (c == '\n') {
				realX = x;
				y += characters[0].height();
			} else if (c == ' '){
				realX += characters[0].height() / 4;
			} else {
				img.put_mask(characters[static_cast<size_t>(c)], realX, y);
				realX += characters[static_cast<size_t>(c)].width() + 1;
			}
		}
	}
private:
	void autoCropX(Image & img){
		int leftX = -1;
		int rightX = -1;
		
		for(int x = 0; x < img.width() && leftX == -1; ++x){
			for(int y = 0; y < img.height(); ++y){
				if (Image::Alpha(img.point_unsafe(x, y)) != 0){
					leftX = x;
					break;
				}
			}
		}
		
		for(int x = img.width() - 1; x > 0 && rightX == -1; --x){
			for(int y = 0; y < img.height(); ++y){
				if (Image::Alpha(img.point_unsafe(x, y)) != 0){
					rightX = x + 1;
					break;
				}
			}
		}
		
		img = img.get_x2_and_y2(leftX, 0, rightX, img.height());
	}

	Image characters[256];
};

#endif
