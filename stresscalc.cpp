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
Font bigfont(Shell::getSelfExecutableDir() + "font.png", -1, -1, false, 1.0f);

inline double catenary(double x, double a) {
    return a * cosh(x / a);
}

double getAForWidth(double width, int height){  //Brute force solver
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


int getMidPointAtHeight(int y, double midPointOfArch_Bottom, double bottomOfArch, double slope){
	const double diffY = static_cast<double>(y) - bottomOfArch;
	const double adjustment = diffY * slope;
	return static_cast<int>(midPointOfArch_Bottom + adjustment);
}

void fixVector(std::vector<std::pair<int, int> > & arches, double midPointOfArch, double bottomOfArch, double slope){
	//Vector needs to go bath and forth
	std::vector<std::pair<int, int> > left;
	std::vector<std::pair<int, int> > right;
	for(auto & p : arches){
		if (p.first < getMidPointAtHeight(p.second, midPointOfArch, bottomOfArch, slope)){
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
	const double bottomOfArch = static_cast<double>(arches[0].second + arches[0].second) / 2.0;
	const double midPointOfArch_Bottom = static_cast<double>(arches[0].first + arches[1].first) / 2.0;
	const double midPointOfArch_Top = static_cast<double>(arches[arches.size() - 1].first + arches[arches.size() - 2].first) / 2.0;
	DEBUG_PLOT_MSG("Top of arch: " << topOfArch);
	DEBUG_PLOT_MSG("Bottom of arch: " << bottomOfArch);
	DEBUG_PLOT_MSG("Midpoint of arch bottom: " << midPointOfArch_Bottom);
	DEBUG_PLOT_MSG("Midpoint of arch top: " << midPointOfArch_Top);
	const double deltaX = midPointOfArch_Top - midPointOfArch_Bottom;
	const double deltaY = topOfArch - bottomOfArch;
	const double slope = deltaX / deltaY;
	fixVector(arches, midPointOfArch_Bottom, bottomOfArch, slope);
	
	copy.rect_fill_x2_and_y2(0, arches[0].second, arches[0].first, arches[0].second + 100, Image::Color(255, 0, 255));
	copy.rect_fill_x2_and_y2(copy.width(), arches[1].second, arches[1].first, arches[1].second + 100, Image::Color(255, 0, 255));
	
	double errorTotal = 0;
	int errorCount = 0;
	
	for(size_t i = 0; i < arches.size() - 2; ++i){	
		if (arches[i].first == 0 && arches[i].second == 0) continue; //Spaceholder
		Image testImage = Image(original.width(), original.height(), Image::Color(255, 255, 255));
		DEBUG_PLOT_MSG("Plotting " << arches[i].first << ", " << arches[i].second);
		const int midForNextCorbel = getMidPointAtHeight(arches[i].second, midPointOfArch_Bottom, bottomOfArch, slope);
		plot(arches[i], midForNextCorbel, topOfArch, testImage, Image::Color(255, 0, 0));
		plot(arches[i], midForNextCorbel, topOfArch, original, Image::Color(255, 0, 0));

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
		int yPos = (i + 2 < arches.size()) ? arches[i + 2].second : static_cast<int>(topOfArch);
		int xPos = (i + 2 < arches.size()) ? arches[i + 2].first : static_cast<int>(midForNextCorbel);
		std::stringstream sss;
		sss << static_cast<int>(stress * 100) << "%";
		
		errorTotal += stress;
		++errorCount;
		if (arches[i].first < midForNextCorbel){
			copy.rect_fill_x2_and_y2(xPos, arches[i].second, 0, yPos, color);
			font.write(sss.str(), copy, 1, yPos);
		} else {
			copy.rect_fill_x2_and_y2(xPos, arches[i].second, copy.width(), yPos, color);
			font.write(sss.str(), copy, copy.width() - 30, yPos);
		}
		
	
		//break;
	}
	
	//Draw the midpoint, see if it's leaning
	for(int yy = static_cast<int>(bottomOfArch); yy >= static_cast<int>(topOfArch); --yy){
		const int xx = getMidPointAtHeight(yy, midPointOfArch_Bottom, bottomOfArch, slope);
		copy.pset(xx, yy, Image::Color(0, 0, 64));
	}
	
	
	plot(arches[0], midPointOfArch_Bottom, topOfArch, copy, Image::Color(255, 255, 0));
	plot(arches[1], midPointOfArch_Bottom, topOfArch, copy, Image::Color(255, 255, 0));
	Image result = Image(original.width() * 2, original.height() + 100, Image::Color(0,0,0));
	result.put(copy, 0, 100);
	result.put(original, original.width(), 100);
	
	std::stringstream sss;
	const int ss = static_cast<int>((100.0f * errorTotal) / static_cast<double>(errorCount));
	const int ssl = static_cast<int>(100.0f * slope);
	sss << "Error: " << ss;
	if (ss > 0){
		sss << "% (Too shallow)" << std::endl;
	} else if (ss < 0){
		sss << "% (Too aggressive)" << std::endl;
	} else {
		sss << "% (Perfect)" << std::endl;
	}
	if (ssl < 0){
		sss << "Lean: " << -ssl << "% Right" << std::endl;
	} else if (ssl > 0){
		sss << "Lean: " << ssl << "% Left" << std::endl;
	} else {
		sss << "Lean: 0%" << std::endl;
	}
	bigfont.write(sss.str(), result, 0, 0);
	
	result.save(output);
	return 0;
}



int main(int argc, char ** argv){
	if (argc < 2){
		std::cerr << "No parameters specified" << std::endl;
		return 255;
	}
	
	std::vector<std::string> files;
	for(int i = 1; i < argc; ++i){
		std::string filename = argv[i];
		DEBUG_PLOT_MSG("Parameter " << i << ": " << filename);
		if (std::filesystem::is_directory(filename)){
			std::vector<std::string> thisfolder = Shell::getFilesInDir(filename, 0);
			for(auto & f : thisfolder){
				std::string ext = Shell::fileExtension(f);
				if (ext == "png") files.push_back(Shell::windowizePaths(Shell::absolutePath(f)));		
			}
		} else {
			std::string ext = Shell::fileExtension(filename);
			if (ext == "png"){  //An image with green dots at the corbels
				files.push_back(Shell::absolutePath(filename));
			} else {
				std::cerr << "Not a PNG or Directory: " << filename << std::endl;
			}
		}
	}
	
	
	DEBUG_PLOT_MSG("Processing " << files.size() << " files");
	for(auto & filename : files){
		std::cout << "Processing " << filename << "..." << std::endl;
		const std::string dirname = Shell::dirname(filename) + "Calculated";
		Shell::mkdir(dirname);
		const std::string output = Shell::windowizePaths(dirname + "\\" + Shell::filename(filename));
		DEBUG_PLOT_MSG("Output: " << output);
		int result = showErrors(Image(filename), output);
		if (result){
			std::cerr << "Error code " << result << std::endl;
		} else {
			std::cout << "Done" << std::endl;
		}
	}
	
	return 0;
}
