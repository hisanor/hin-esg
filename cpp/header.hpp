// Header
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <unordered_map>
#include <string>
#include <regex>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/regex.hpp>
#include "json.hpp"
using json = nlohmann::json;

// Using bimap
typedef boost::bimaps::bimap<std::string,std::string> bimap;
typedef bimap::value_type bimap_val;

//// REVISED ////////

// Utility Function
//std::string split
std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);}
    return elems;}

std::vector<std::string> Split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    Split(s, delim, elems);
    return elems;}


std::vector<std::string> SplitDqcsv(std::string str){
    std::vector<std::string> v1;
    const char *mystart=str.c_str();    // prepare to parse the line - start is position of begin of field
    bool instring{false};
    for (const char* p=mystart; *p; p++) {  // iterate through the string
        if (*p=='"')                        // toggle flag if we're btw double quote
            instring = !instring;
        else if (*p==',' && !instring) {    // if comma OUTSIDE double quote
            v1.push_back(std::string(mystart,p-mystart));  // keep the field
            mystart=p+1;                    // and start parsing next one
        }
    }
    v1.push_back(std::string(mystart));   // last field delimited by end of line instead of comma
    return(v1);
}


std::vector<int> Split2IntVector(std::string line){
    std::vector<std::string> elements0 = Split(line,',');
    std::vector<int> elements;
    for(int i=0;i<elements0.size();++i){
        elements.push_back(boost::lexical_cast<int>(elements0[i]));
    }
    return(elements);
}

/*
std::vector<int64_t> Split2LongVector(std::string line){
    std::vector<std::string> elements0 = Split(line,',');
    std::vector<int64_t> elements;
    for(int i=0;i<elements0.size();++i){
        elements.push_back(boost::lexical_cast<int64_t>(elements0[i]));
    }
    return(elements);
}
/****/

std::vector<long long> Split2LongVector(std::string line){
    std::vector<std::string> elements0 = Split(line,',');
    std::vector<long long> elements;
    for(int i=0;i<elements0.size();++i){
        elements.push_back(boost::lexical_cast<long long>(elements0[i]));
    }
    return(elements);
}
/****/

std::string Trimstring(const std::string s) {//stringの左右両端の、タブ、改行を消す
    if(s.length() == 0)
        return s;
    int b = s.find_first_not_of(" \t\r\n,");
    int e = s.find_last_not_of(" \t\r\n,");
    if(b == -1)// 左右両端に、スペース、タブ、改行がない。
        return "";
    return std::string(s, b, e - b + 1);}

std::string Trimquote(const std::string s) {//trimquote
    if(s.length() == 0)
        return s;
    int b = s.find_first_not_of("\"");
    int e = s.find_last_not_of("\"");
    if(b == -1)
        return "";
    return std::string(s, b, e - b + 1);}

float Sigmoid(float x){
    return( float(1.0) / (float(1.0) + exp(-x)));
    //return(0.5 * (x / (1 + abs(x)) + 0.5);
}

float DerSigmoid(float x){
    return(Sigmoid(x) * (float(1.0) - Sigmoid(x)));
}

float TanH(float x){
    float numer = exp(x) - exp(-x);
    float denom = exp(x) + exp(-x);
    return(numer/denom);
}

float DerTanH(float x){
    float out = float(1.0) - (TanH(x)*TanH(x));
    return(out);
}


float SigmoidFast(float x){
    //return(1 / ( 1+exp(-x)));
    return(0.5 * (x / (1 + abs(x)) + 0.5));
}

float Relu(float x){
    return(log(1 + exp(x)));
}

double ContorPair(double x1,double x2){
    x1 += 1.0;
    x2 += 1.0;
    return((x1+x2)*(x1+x2+1.0)/2.0 + x2);
}

double ContorTuple(double x1,double x2,double x3){
    x1 += 1.0;
    x2 += 1.0;
    x3 += 1.0;
    return(ContorPair(ContorPair(x1,x2),x3));
}

// Draw Dirichlet Sample
void RandomDirichlet(std::mt19937_64 &mt,std::vector<float> &x, const float alpha) {
    std::gamma_distribution<float> gamma(alpha, 1.0);
    float sum_y = 0;
    for(int i = 0; i < x.size(); i++){
        float y = gamma(mt);
            sum_y += y;
            x[i] = y;}
    std::for_each(x.begin(), x.end(), [sum_y](float &v) { v /= sum_y; });
}


int EditDistance(const std::string &s1, const std::string &s2){
  	// To change the type this function manipulates and returns, change
  	// the return type and the types of the two variables below.
  	int s1len = s1.size();
  	int s2len = s2.size();
  	auto column_start = (decltype(s1len))1;
  	auto column = new decltype(s1len)[s1len + 1];
  	std::iota(column + column_start - 1, column + s1len + 1, column_start - 1);
  	for (auto x = column_start; x <= s2len; x++) {
  		column[0] = x;
  		auto last_diagonal = x - column_start;
  		for (auto y = column_start; y <= s1len; y++) {
  			auto old_diagonal = column[y];
  			auto possibilities = {
  				column[y] + 1,
  				column[y - 1] + 1,
  				last_diagonal + (s1[y - 1] == s2[x - 1]? 0 : 1)
  			};
  			column[y] = std::min(possibilities);
  			last_diagonal = old_diagonal;
  		}
  	}
  	auto result = column[s1len];
  	delete[] column;
  	return result;
}

// trimquote
std::string TrimSemi(const std::string s) {//trimquote
  if(s.length() == 0)
    return s;
  int b = s.find_first_not_of(";");
  int e = s.find_last_not_of(";");
  if(b == -1)
    return "";
  return std::string(s, b, e - b + 1);
}


// trimquote
std::string TrimComma(const std::string s) {//trimquote
  if(s.length() == 0)
    return s;
  int b = s.find_first_not_of(",");
  int e = s.find_last_not_of(",");
  if(b == -1)
    return "";
  return std::string(s, b, e - b + 1);
}



// std::string replace
void StrReplace(std::string &str,const std::string &from,const std::string &to){
  std::string::size_type pos = 0;
  while(pos = str.find(from,pos),pos!=std::string::npos){
      str.replace(pos,from.length(),to);
      pos += to.length();}
}

bool BothAreSpaces(char lhs, char rhs) {
  return (lhs == rhs) && (lhs == ' ');
}

void RemoveDoubleSpaces(std::string &story){
    std::string::iterator new_end = std::unique(story.begin(),story.end(), BothAreSpaces);
    story.erase(new_end,story.end());
}




//// NOT YET REVISED ////////



// sort indexes
template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T> &v) {
/*
 * Descr: place any std::vector and it would return an indices
 * Input: std::vector
 * Output: Indices vector
 /***/
    // initialize original index locations
    std::vector<size_t> idx(v.size());
    iota(idx.begin(), idx.end(), 0);
    // sort indexes based on comparing values in v
    sort(idx.begin(), idx.end(),
       [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
    return idx;
}

// Remove control sequence
std::string remove_ctrl(std::string s){
	std::string result;
	for(int i=0;i<s.length();++i){
		if(s[i] >=0x20){
			result = result + s[i];}}
	return result;}








// join std::string
std::string join_str(std::vector<std::string> v1,const char* delim){
/* Descr: C++ substitute of python's str.join method
 * Input: vector of std::string
 * Output: joined string
/***/
    std::ostringstream  os;
    std::copy(v1.begin(), v1.end(), std::ostream_iterator<std::string>(os,delim));
    std::string outstr = Trimstring(os.str());
    return(outstr);
}

// convert date
std::string conv_date (std::string str){
    std::string y = "";
    std::string m = "";
    std::string d = "";
    for(int i=0;i<4;++i) y += str[i];
    for(int i=4;i<6;++i) m += str[i];
    for(int i=6;i<8;++i) d += str[i];
    std::string ymd = y + "-" + m + "-" + d;
    return ymd;
}

// lastquart
std::string lastquart(std::string str){
    std::vector<std::string> v1 = Split(str,'-');
    int y = boost::lexical_cast<int>(v1[0]);
    int m = boost::lexical_cast<int>(v1[1]);
    m = m - 3;
    if(m < 1){
        m = 12 + m;
        y = y - 1;}
    std::string mstr = "";
    if(m < 10){
        mstr = "0" + boost::lexical_cast<std::string>(m);
    }else{
        mstr = boost::lexical_cast<std::string>(m);}
    std::string dstr="";
    if(m==1 || m==3 || m==5 || m==7 || m==8 || m==10 || m==12) dstr = "31";
    if(m==4 || m==6 || m==9 || m==11) dstr="30";
    if(m==2 && y % 4 !=0) dstr="28";
    if(m==2 && y % 4 ==0) dstr="29";
    std::string outstr = boost::lexical_cast<std::string>(y) + "-" + mstr + "-" + dstr;
    return(outstr);
}

//returns current time in seconds
double current_time () {
    timeval tv;
    gettimeofday(&tv, NULL);
    double rtn_value = (double) tv.tv_usec;
    rtn_value /= 1e6;
    rtn_value += (double) tv.tv_sec;
    return rtn_value;}

// No Double Space
std::string no_d_space(std::string line){
    int loc = 0;
    while((loc = line.find("  ",loc)) != std::string::npos) //Two spaces here
    {
       line.replace(loc,2," "); //Single space in quotes
    }
    return line;
}

std::string no_quote (std::string line){//FAST
    int loc = 0;
    while((loc = line.find("\"",loc)) != std::string::npos) line.replace(loc,1,"");
    return line;
}

// Probably Used Only Here
std::string no_quote_plus (std::string str){
    std::regex re("\\\\|\"|,|;");
    str = std::regex_replace(str,re,"");
    return str;
}

// Probably Used Only Here
std::string no_quote_plus2 (std::string str){
    std::regex re("\"|\'|\\(|\\)|\\[|\\]");
    str = std::regex_replace(str,re,"");
    return str;
}

// To cope with text problems creates by windows
std::string remove_r(std::string str){
    str.erase( std::remove(str.begin(), str.end(), '\r'), str.end() );
    return(str);
}

// Probably Used Only Here
std::string no_http (std::string str){
    std::regex re("http://|https://");
    str = std::regex_replace(str,re,"");
    return str;
}
