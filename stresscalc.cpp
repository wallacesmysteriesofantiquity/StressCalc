#include "./Graphics/Image.h"
#include "./Graphics/Font.h"
#include "./Utils/Shell.h"
#include <cmath>
#include <cassert>


//#define DEBUG_PLOT

#ifdef DEBUG_PLOT
	#define DEBUG_PLOT_MSG(x) std::cout << x << std::endl;
	#define DEBUG_PLOT_CODE(x) x
#else
	#define DEBUG_PLOT_MSG(x)
	#define DEBUG_PLOT_CODE(x)
#endif

Font font(Shell::getSelfExecutableDir() + "font.png", -1, -1, false, 0.5f);

inline double catenary(double x, double a) {
    return a * cosh(x / a);
}

double getAForWidth(double width, int height){
	double a = 1.0;
	double increaser = 0.1;
	while(a < 100.0){
		double y = catenary(width / 2.0, a);
		y -= catenary(0.0, a);
		if (y < height){
			a -= increaser;
			if (increaser < 0.000001) return a;
			increaser /= 2.0;
		}
		a += increaser;
	}
	std::cerr << "Was not able to figure out a value for " << width << ", " << height << std::endl;
	return 0;
}


void plot(const std::pair<int, int> & point, double midPointOfArch, double topOfArch, Image & testImage, uint32_t col){
	//std::cout << "Plotting " << x1 << ", " << x2 << ", " << topOfArch << ", " << bottomOfArch << std::endl;
	assert(point.second > topOfArch);

	const double width = fabs(midPointOfArch - point.first) * 2.0;
	const int height = point.second - static_cast<int>(topOfArch);
	double a = getAForWidth(width, height);
	DEBUG_PLOT_MSG("W: " << width << ", H: " << height << ", a: " << a);
	double yAdjust = catenary(0.0, a);
	double yLast = topOfArch;
    for (int x = 0; x <= static_cast<int>(width / 2.0); ++x) {
        double y = catenary(static_cast<double>(x), a);
		const double nextY = y - yAdjust + topOfArch;
		if (point.first < midPointOfArch){
			testImage.line(static_cast<int>(midPointOfArch) - x, static_cast<int>(nextY), static_cast<int>(midPointOfArch) - (x - 1), static_cast<int>(yLast), col);
		} else {
			testImage.line(static_cast<int>(midPointOfArch) + x, static_cast<int>(nextY), static_cast<int>(midPointOfArch) + (x - 1), static_cast<int>(yLast), col);
		}
		yLast = y - yAdjust + topOfArch;
    }
	
	if (point.first < midPointOfArch){
		testImage.line(point.first, point.second, static_cast<int>(midPointOfArch) - (static_cast<int>(width / 2.0)), static_cast<int>(yLast), col);
	} else {
		testImage.line(point.first, point.second, static_cast<int>(midPointOfArch) + (static_cast<int>(width / 2.0)), static_cast<int>(yLast), col);
	}
}

std::vector<std::pair<int, int> > getAllArches(const Image & testImage){
	std::vector<std::pair<int, int> > result;
	
	for(int y = testImage.height(); y >= 0; --y){
		for(int x = 0; x < testImage.width(); ++x){
			uint32_t col = testImage.point(x, y);
			if (Image::Green(col) == 255 && Image::Red(col) == 0 && Image::Blue(col) == 0){
				result.push_back(std::pair<int, int>(x, y));
			}
		}
	}
	
	return result;
}


void fixVector(std::vector<std::pair<int, int> > & arches, double midPointOfArch){
	//Vector needs to go bath and forth
	std::vector<std::pair<int, int> > left;
	std::vector<std::pair<int, int> > right;
	for(auto & p : arches){
		if (p.first < midPointOfArch){
			left.push_back(p);
		} else {
			right.push_back(p);
		}
	}
	
	//If there are more blocks on one side, that's fine, just insert zeros
	std::vector<std::pair<int, int> >::iterator iLeft = left.begin();
	std::vector<std::pair<int, int> >::iterator iRight = right.begin();
	std::vector<std::pair<int, int> > result;
	while(iLeft != left.end() && iRight != right.end()){
		if (iLeft != left.end()){
			result.push_back(*iLeft);
			++iLeft;
		} else {
			result.push_back(std::pair<int, int>(0,0));
		}
		if (iRight != right.end()){
			result.push_back(*iRight);
			++iRight;
		} else {
			result.push_back(std::pair<int, int>(0,0));
		}
	}
	arches = result;
	
	DEBUG_PLOT_CODE(for(auto & p : arches) DEBUG_PLOT_MSG(p.first << ", " << p.second));
}







int showErrors(Image original, const std::string & output){
	Image copy(original);
	std::vector<std::pair<int, int> > arches = getAllArches(original);
	if (arches.size() < 4){
		std::cerr << "Didn't find enough block markers" << std::endl;
		return 1;
	}
	
	//Figure out exactly how high the arch is and where the midpoint is
	const double topOfArch = static_cast<double>(arches[arches.size() - 1].second + arches[arches.size() - 2].second) / 2.0;
	const double midPointOfArch = static_cast<double>(arches[0].first + arches[1].first) / 2.0;
	DEBUG_PLOT_MSG("Top of arch: " << topOfArch);
	DEBUG_PLOT_MSG("Midpoint of arch: " << midPointOfArch);
	fixVector(arches, midPointOfArch);
	
	copy.rect_fill_x2_and_y2(0, arches[0].second, arches[0].first, arches[2].second, Image::Color(255, 0, 255));
	copy.rect_fill_x2_and_y2(copy.width(), arches[1].second, arches[1].first, arches[3].second, Image::Color(255, 0, 255));
	
	double errorTotal = 0;
	int errorCount = 0;
	
	for(size_t i = 0; i < arches.size() - 4; ++i){	
		if (arches[i].first == 0 && arches[i].second == 0) continue; //Spaceholder
		Image testImage = Image(original.width(), original.height(), Image::Color(255, 255, 255));
		DEBUG_PLOT_MSG("Plotting " << arches[i].first << ", " << arches[i].second);
		plot(arches[i], midPointOfArch, topOfArch, testImage, Image::Color(255, 0, 0));
		plot(arches[i], midPointOfArch, topOfArch, original, Image::Color(255, 0, 0));

		//Since the corbels will go back and forth, the next corbel is actually +2
		int xSearch = arches[i + 2].first;
		int ySearch = arches[i + 2].second;
		int error = 0;
		for(int j = 0; j < original.width(); ++j){
			uint32_t c1 = testImage.point(xSearch + j, ySearch);
			
			testImage.pset(xSearch + j, ySearch, Image::Color(0, 255, 128));
			if (Image::Green(c1) == 0 && Image::Blue(c1) == 0 && Image::Red(c1) == 255){
				error = j;
				break;
			} else {
				uint32_t c2 = testImage.point(xSearch - j, ySearch);
				testImage.pset(xSearch - j, ySearch, Image::Color(0, 128, 255));
				if (Image::Green(c2) == 0 && Image::Blue(c2) == 0 && Image::Red(c2) == 255){
					error = -j;
					break;
				}
			}				
		}
		
		//testImage.save(output);
		
		double overhang = static_cast<double>(arches[i + 2].first - arches[i].first);
		double stress = static_cast<double>(error) / overhang;
		if (overhang == 0.0) stress = 2.0;
		DEBUG_PLOT_MSG("Actual overhang: " << overhang);
		DEBUG_PLOT_MSG("Error from ideal: " << error);
		uint32_t color = 0;
		if (stress < 0){  //Too aggressive
			DEBUG_PLOT_MSG("Too aggressive, Stress: " << stress);
			color = Image::ColorBetween(Image::Color(255, 0, 255), Image::Color(255, 0, 0), static_cast<float>(-stress / 2.0));
		} else {  //Too shallow
			DEBUG_PLOT_MSG("Too shallow, Stress: " << stress);
			color = Image::ColorBetween(Image::Color(255, 0, 255), Image::Color(0, 0, 255),  static_cast<float>(stress / 2.0));
		}
		int yPos = (i + 4 < arches.size()) ? arches[i + 4].second : static_cast<int>(topOfArch);
		int xPos = (i + 4 < arches.size()) ? arches[i + 4].first : static_cast<int>(midPointOfArch);
		std::stringstream sss;
		sss << static_cast<int>(stress * 100) << "%";
		
		errorTotal += stress;
		++errorCount;
		if (arches[i].first < midPointOfArch){
			copy.rect_fill_x2_and_y2(xPos, arches[i + 2].second, 0, yPos, color);
			font.write(sss.str(), copy, 1, yPos);
		} else {
			copy.rect_fill_x2_and_y2(xPos, arches[i + 2].second, copy.width(), yPos, color);
			font.write(sss.str(), copy, copy.width() - 30, yPos);
		}
		
	
		//break;
	}
	
	
	plot(arches[0], midPointOfArch, topOfArch, copy, Image::Color(255, 255, 0));
	plot(arches[1], midPointOfArch, topOfArch, copy, Image::Color(255, 255, 0));
	Image result = Image(original.width() * 2, original.height(), Image::Color(0,0,0));
	result.put(copy, 0, 0);
	result.put(original, original.width(), 0);
	
	std::stringstream sss;
	const int ss = static_cast<int>((100.0f * errorTotal) / static_cast<double>(errorCount));
	sss << "Average Stress: " << ss;
	if (ss > 0){
		sss << "% (Too shallow)" << std::endl;
	} else {
		sss << "% (Too aggressive)" << std::endl;
	}
	font.write(sss.str(), result, 0, 0);
	
	result.save(output);
	return 0;
}



int main(int argc, char ** argv){
	if (argc < 2){
		std::cerr << "No parameters specified" << std::endl;
		return 255;
	}
	for(int i = 1; i < argc; ++i){
		std::string filename = argv[i];
		std::string ext = Shell::fileExtension(filename);
		if (ext == "png"){  //An image with green dots at the corbels
			std::string dirname = filename.substr(0, filename.length() - (ext.length() + 1));
			std::string output = dirname + "_stress.png";
			int result = showErrors(Image(filename), output);
			if (result) return result;
		} else if (ext == "txt"){  //A text file: width, heigt left, height right, corbel left, corbel right...
			std::cerr << "TXT file code not created: " << filename << std::endl;
			return 255;
		} else {
			std::cerr << "Not a PNG or TXT file: " << filename << std::endl;
			return 255;
		}
	}
	return 0;
}