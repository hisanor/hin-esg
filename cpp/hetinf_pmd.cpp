#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <stdint.h>
#include <regex>
#include <omp.h>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
using namespace boost::interprocess;
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/operators.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>
#include <pybind11/eigen.h>
//using namespace pybind11::literals;
namespace py = pybind11;
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <json.hpp>
using json = nlohmann::json;
#include "header.hpp"

// OMAJINAI: Opaque operation makes pass-by-reference possible
using StringVector = std::vector<std::string>;
using IntVector = std::vector<int>;
using FloatVector = std::vector<float>;
using StringIntMap = std::unordered_map<std::string,int>;
using IntStringMap = std::unordered_map<int,std::string>;
using StringDoubleMap = std::unordered_map<std::string,double>;
using StringStringMap = std::unordered_map<std::string,std::string>;
using IntStringMultiMap = std::unordered_multimap<int,std::string>;
using StringIntMultiMap = std::unordered_multimap<std::string,int>;
using IntIntMap = std::unordered_map<int,int>;
using IntIntMultiMap = std::unordered_multimap<int,int>;
using IntVectorMap = std::unordered_map<int,std::vector<int>>;
using LongVector = std::vector<long long>;
using StringLongMap = std::unordered_map<std::string,long long>;
using LongStringMap = std::unordered_map<long long,std::string>;
using LongStringMultiMap = std::unordered_multimap<long long,std::string>;
using StringLongMultiMap = std::unordered_multimap<std::string,long long>;
using LongLongMap = std::unordered_map<long long,long long>;
using LongLongMultiMap = std::unordered_multimap<long long,long long>;
using LongVectorMap = std::unordered_map<long long,std::vector<long long>>;
PYBIND11_MAKE_OPAQUE(StringVector);
PYBIND11_MAKE_OPAQUE(IntVector);
PYBIND11_MAKE_OPAQUE(FloatVector);
PYBIND11_MAKE_OPAQUE(StringIntMap);
PYBIND11_MAKE_OPAQUE(IntStringMap);
PYBIND11_MAKE_OPAQUE(StringDoubleMap);
PYBIND11_MAKE_OPAQUE(IntStringMultiMap);
PYBIND11_MAKE_OPAQUE(StringIntMultiMap);
PYBIND11_MAKE_OPAQUE(IntIntMap);
PYBIND11_MAKE_OPAQUE(IntIntMultiMap);
PYBIND11_MAKE_OPAQUE(IntVectorMap);
PYBIND11_MAKE_OPAQUE(LongVector);
PYBIND11_MAKE_OPAQUE(StringLongMap);
PYBIND11_MAKE_OPAQUE(LongStringMap);
PYBIND11_MAKE_OPAQUE(LongStringMultiMap);
PYBIND11_MAKE_OPAQUE(StringLongMultiMap);
PYBIND11_MAKE_OPAQUE(LongLongMap);
PYBIND11_MAKE_OPAQUE(LongLongMultiMap);
PYBIND11_MAKE_OPAQUE(LongVectorMap);
PYBIND11_MAKE_OPAQUE(StringStringMap);
/**END OMAJINAI**/

class Nodes {// Nodes
    public:
    std::string name;
    Nodes (const std::string &name0){// Constructor
        name = name0;
    }

    // Objects
    std::vector<std::string> node_filelist,node_pathlist,header_file,header_name;
    std::vector<std::string> extract_files,extract_variables;// Use to extract node-label info
    std::unordered_map<std::string,std::unordered_map<std::string,std::string>> node_variables;
    std::unordered_map<std::string,std::unordered_map<std::string,std::string>> node_variables2;
    std::vector<std::string> node_vector,value_vector,variable_name_vector;
    std::unordered_map<std::string,std::string> node2name;
    json jnodes;

    void ParseHeader(){
        header_file.clear();
        header_file.shrink_to_fit();
        header_name.clear();
        header_name.shrink_to_fit();
        std::string str;
        double n = 0.0;
        std::ifstream file("/home/rh/Arbeitsraum/Files/KG/All/serial/NodeFiles.txt");
        while(std::getline(file,str)){
            node_filelist.push_back(Trimstring(str));
            std::string file_path =  "/home/rh/Arbeitsraum/Files/KG/All/" + (Trimstring(str));
            node_pathlist.push_back(file_path);
        }
        for(int i=0;i<node_pathlist.size();++i){
            n = 0;
            std::ifstream file(node_pathlist[i],std::ios_base::in | std::ios_base::binary);
            try{
                boost::iostreams::filtering_istream in;
                in.push(boost::iostreams::gzip_decompressor());
                in.push(file);
                for(std::string str; std::getline(in, str);){
                    if(n>0){
                        break;
                    }else{
                        std::vector<std::string> v1 = SplitDqcsv(str);
                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            header_name.push_back(v1[j]);
                            header_file.push_back(node_filelist[i]);
                        }
                    }
                    n += 1;
                }
            }catch(const boost::iostreams::gzip_error& e){
                std::cout << e.what() << std::endl;
            }
        }
    }

    void ClearExtractFiles(){
        extract_files.clear();extract_files.shrink_to_fit();
        extract_variables.clear();extract_variables.shrink_to_fit();
    }

    void ParseFile(){
        node_variables.clear();
        for(int i=0;i<extract_files.size();++i){
            double line_counter = 0;
            std::string fname = "/home/rh/Arbeitsraum/Files/KG/All/" + extract_files[i];
            std::ifstream file(fname);
            std::string variable_name = extract_variables[i];
            try{
                boost::iostreams::filtering_istream in;
                in.push(boost::iostreams::gzip_decompressor());
                in.push(file);
                int place_id = 0,place_variable = 0,place_name = 0;
                for(std::string str; std::getline(in,str);){
                    std::vector<std::string> v1 = SplitDqcsv(str);
                    if(line_counter==0){// First Line
                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == variable_name){
                                place_variable = j;
                                break;}}
                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == "ID:ID"){
                                place_id = j;
                                break;}}

                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == "name" ){
                                place_name = j;
                                break;
                            }
                        }
                    }else{
                        v1[place_id] = Trimquote(v1[place_id]);
                        v1[place_variable] = Trimquote(v1[place_variable]);
                        v1[place_name] = Trimquote(v1[place_name]);
                        if(v1[place_variable]!=""){// if value exists
                            std::unordered_map<std::string,std::string> tmap;
                            if(place_name!=place_id){
                                tmap["name"] = v1[place_name];
                            }
                            tmap[variable_name] = v1[place_variable];// This is the new dict we wnt add
                            if(node_variables.find(v1[place_id])==node_variables.end()){// prev not exst
                                node_variables[v1[place_id]] = tmap;
                                node2name[v1[place_id]] = v1[place_name];
                            }else{
                                std::unordered_map<std::string,std::string> tmap2;
                                tmap2 = node_variables[v1[place_id]];// load prev
                                if(tmap2.find(variable_name)==tmap2.end()){
                                    tmap2[variable_name] = v1[place_variable];
                                }else{
                                    //std::cout << "OH my GOD" << std::endl;// WE USUALLY DONT COME HERE
                                    std::string vname2 = extract_files[i] + "_" + variable_name;
                                    tmap2[vname2] = v1[place_variable];
                                }
                                node_variables[v1[place_id]] = tmap2;
                            }
                        }
                    }
                    line_counter += 1.0;
                }
            }catch(const boost::iostreams::gzip_error& e){
                std::cout << e.what() << std::endl;
            }
        }
    }

    void ParseFile2(std::string &cond_variable){
        node_variables.clear();
        for(int i=0;i<extract_files.size();++i){
            double line_counter = 0;
            std::string fname = "/home/rh/Arbeitsraum/Files/KG/All/" + extract_files[i];
            std::ifstream file(fname);
            std::string variable_name = extract_variables[i];
            try{
                boost::iostreams::filtering_istream in;
                in.push(boost::iostreams::gzip_decompressor());
                in.push(file);
                int place_id = 0,place_variable = 0,place_name = 0;
                for(std::string str; std::getline(in,str);){
                    std::vector<std::string> v1 = SplitDqcsv(str);
                    if(line_counter==0){// First Line
                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == variable_name){
                                place_variable = j;
                                break;}}
                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == "ID:ID"){
                                place_id = j;
                                break;}}

                        for(int j=0;j<v1.size();++j){
                            v1[j] = Trimquote(v1[j]);
                            if(v1[j] == "name" ){
                                place_name = j;
                                break;
                            }
                        }
                    }else{
                        v1[place_id] = Trimquote(v1[place_id]);
                        v1[place_variable] = Trimquote(v1[place_variable]);
                        v1[place_name] = Trimquote(v1[place_name]);
                        if(v1[place_variable]!=""){// if value exists
                            std::unordered_map<std::string,std::string> tmap;
                            if(place_name!=place_id){
                                tmap["name"] = v1[place_name];
                            }
                            tmap[variable_name] = v1[place_variable];// This is the new dict we wnt add
                            if(node_variables.find(v1[place_id])==node_variables.end()){// prev not exst
                                node_variables[v1[place_id]] = tmap;
                                node2name[v1[place_id]] = v1[place_name];
                            }else{
                                std::unordered_map<std::string,std::string> tmap2;
                                tmap2 = node_variables[v1[place_id]];// load prev
                                if(tmap2.find(variable_name)==tmap2.end()){
                                    tmap2[variable_name] = v1[place_variable];
                                }else{
                                    //std::cout << "OH my GOD" << std::endl;// WE USUALLY DONT COME HERE
                                    std::string vname2 = extract_files[i] + "_" + variable_name;
                                    tmap2[vname2] = v1[place_variable];
                                }
                                node_variables[v1[place_id]] = tmap2;
                            }
                        }
                    }
                    line_counter += 1.0;
                }
            }catch(const boost::iostreams::gzip_error& e){
                std::cout << e.what() << std::endl;
            }
        }
    }

    void TrimNodeVariables(const std::string must_variable){
        node_variables2.clear();
        for(auto itr=node_variables.begin();itr!=node_variables.end();++itr){
            std::unordered_map<std::string,std::string> temp_map = node_variables[itr->first];
            int hantei = 0;
            if(must_variable!=""){
                if(temp_map.find(must_variable)!=temp_map.end()){
                    std::string temp_string = temp_map[must_variable];
                    std::cout << temp_string << std::endl;
                    hantei = 1;
                }
            }else{
                hantei = 1;
            }
            if(hantei == 1){
                node_variables2[itr->first] = node_variables[itr->first];
            }
        }
    }

    std::unordered_map<std::string,std::string> node_variable2value;
    void Melt(){
        node_id.clear();node_id.shrink_to_fit();
        node_variable2value.clear();
        for(auto itr=node_variables.begin();itr!=node_variables.end();++itr){
            std::unordered_map<std::string,std::string> tempmap = node_variables[itr->first];
            for(auto itr2=tempmap.begin();itr2!=tempmap.end();++itr2){
                std::string node_var = itr->first + "," + itr2->first;
                node_variable2value[node_var] = itr2->second;
            }
        }
    }

    bool IsNumber(const std::string& s){
       return !s.empty() && s.find_first_not_of("-.0123456789") == std::string::npos;
    }

    std::vector<std::string> node_id;
    std::vector<double> longitude,latitude;
    void GetLongitudeLatitude(){
        for(auto itr=node_variables.begin();itr!=node_variables.end();++itr){
            std::string longitude1,latitude1,longitude2,latitude2;
            std::string temp_string = itr->first + "," + "reuters_longitude:double";
            std::string temp_string2 = itr->first + "," + "reuters_latitude:double";
            if(node_variable2value.find(temp_string)!=node_variable2value.end()){
                longitude1 = node_variable2value.at(temp_string);
                latitude1 =  node_variable2value.at(temp_string2);
            }
            temp_string = itr->first + "," + "factset_longitude:double";
            temp_string2 = itr->first + "," + "factset_latitude:double";
            if(node_variable2value.find(temp_string)!=node_variable2value.end()){
                longitude2 = node_variable2value.at(temp_string);
                latitude2 =  node_variable2value.at(temp_string2);
            }
            if(longitude1!="-1.0" && longitude1!="0" && IsNumber(longitude1) && IsNumber(latitude1)){
                node_id.push_back(itr->first);
                longitude.push_back(boost::lexical_cast<double>(longitude1));
                latitude.push_back(boost::lexical_cast<double>(latitude1));
            }else if(longitude2!="-1.0" && longitude2!="0" && IsNumber(longitude2) && IsNumber(latitude2)){
                node_id.push_back(itr->first);
                longitude.push_back(boost::lexical_cast<double>(longitude2));
                latitude.push_back(boost::lexical_cast<double>(latitude2));
            }
        }
    }

    std::vector<std::string> core_nodes;
    void ClearCoreNodes(){
        core_nodes.clear();
        core_nodes.shrink_to_fit();
    }

    std::vector<std::string> node_vec,name_vec,label1_vec,label2_vec,start_vec,ticker_vec;

    void CreateTickerDataFrame(){
        node_vec.clear();node_vec.shrink_to_fit();
        name_vec.clear();name_vec.shrink_to_fit();
        ticker_vec.clear();ticker_vec.shrink_to_fit();
        std::unordered_map<std::string,int> node_id_ticker_indicator;
        std::string node_id_ticker;
        std::string node_id;
        std::string node_name;
        std::string node_name_string;
        std::regex re1("\\([\\S]*?\\)");
        std::string text("Dog is nice. (Dog) is cute. Dog is wr:awesome . fgfd") ;
        std::string result = std::regex_replace(text,re1,"");
        std::cout << result << std::endl;

        std::regex re2("([\\S]*?:[\\S]*?)[\\s\\.]");
        std::string result2 =std::regex_replace(result,re2,"");
        std::cout << result2 << std::endl;

        for(int i=0;i<core_nodes.size();++i){
            node_id = core_nodes[i];
            // Order 1(d,c,f):2932
            if(node_variables[node_id].find("name")!=node_variables[node_id].end()){
                std::string temp_string = node_variables[node_id]["name"];
                if(temp_string != ""){
                    node_name_string = temp_string;
                }
            }
            if(node_variables[node_id].find("djrc_name")!=node_variables[node_id].end()){
                std::string temp_string = node_variables[node_id]["djrc_name"];
                if(temp_string != ""){
                    node_name_string = temp_string;
                }
            }
            if(node_variables[node_id].find("capitaliq_name")!=node_variables[node_id].end()){
                std::string temp_string = node_variables[node_id]["capitaliq_name"];
                if(temp_string != ""){
                    node_name_string = temp_string;
                }
            }
            if(node_variables[node_id].find("factset_name")!=node_variables[node_id].end()){
                std::string temp_string = node_variables[node_id]["factset_name"];
                if(temp_string != ""){
                    node_name_string = temp_string;
                }
            }
            std::vector<std::string> vec_node_id = Split(node_name_string,';');
            node_name = vec_node_id[0];
            node_name = node_name + " ";
            node_name = std::regex_replace(node_name,re1,"");
            node_name = std::regex_replace(node_name,re2,"");
            RemoveDoubleSpaces(node_name);

            // KOKO
            std::string ticker_string = node_variables[node_id]["factset_ticker"];
            std::vector<std::string> vec_ticker = Split(ticker_string,';');
            for(int j=0;j<vec_ticker.size();++j){
                node_id_ticker = node_id + "," + vec_ticker[j];
                if(node_id_ticker_indicator.find(node_id_ticker) == node_id_ticker_indicator.end() && vec_ticker[j]!=""){
                    node_id_ticker_indicator[node_id_ticker] = 1;
                    node_vec.push_back(node_id);
                    name_vec.push_back(node_name);
                    ticker_vec.push_back(vec_ticker[j]);
                }
            }
            ticker_string = node_variables[node_id]["capitaliq_ticker"];
            vec_ticker = Split(ticker_string,';');
            for(int j=0;j<vec_ticker.size();++j){
                node_id_ticker = node_id + "," + vec_ticker[j];
                if(node_id_ticker_indicator.find(node_id_ticker) == node_id_ticker_indicator.end() && vec_ticker[j]!=""){
                    node_id_ticker_indicator[node_id_ticker] = 1;
                    node_vec.push_back(node_id);
                    name_vec.push_back(node_name);
                    ticker_vec.push_back(vec_ticker[j]);
                    //std::cout << "CapitalIQ Ticker: " << vec_ticker[j] << std::endl;
                }
            }
            ticker_string = node_variables[node_id]["reuters_ticker"];
            vec_ticker = Split(ticker_string,';');
            for(int j=0;j<vec_ticker.size();++j){
                node_id_ticker = node_id + "," + vec_ticker[j];
                if(node_id_ticker_indicator.find(node_id_ticker) == node_id_ticker_indicator.end() && vec_ticker[j]!=""){
                    node_id_ticker_indicator[node_id_ticker] = 1;
                    node_vec.push_back(node_id);
                    name_vec.push_back(node_name);
                    ticker_vec.push_back(vec_ticker[j]);
                }
            }
            ticker_string = node_variables[node_id]["ticker"];
            vec_ticker = Split(ticker_string,';');
            for(int j=0;j<vec_ticker.size();++j){
                node_id_ticker = node_id + "," + vec_ticker[j];
                if(node_id_ticker_indicator.find(node_id_ticker) == node_id_ticker_indicator.end() && vec_ticker[j]!=""){
                    node_id_ticker_indicator[node_id_ticker] = 1;
                    node_vec.push_back(node_id);
                    name_vec.push_back(node_name);
                    ticker_vec.push_back(vec_ticker[j]);
                }
            }
        }
    }

    void CreateLabelDataFrame(){
        node_vec.clear();node_vec.shrink_to_fit();
        name_vec.clear();name_vec.shrink_to_fit();
        label1_vec.clear();label1_vec.shrink_to_fit();
        label2_vec.clear();label2_vec.shrink_to_fit();
        start_vec.clear();start_vec.shrink_to_fit();
        for(int i=0;i<core_nodes.size();++i){
            std::string node_id = core_nodes[i];

            std::string node_name = "";
            if(node_variables[node_id].find("name")!=node_variables[node_id].end()){
                if(node_variables[node_id]["name"] !=""){
                    node_name = node_variables[node_id]["name"];
                }
            }
            if(node_name == ""){
                if(node_variables[node_id].find("djrc_name")!=node_variables[node_id].end()){
                    if(node_variables[node_id]["djrc_name"] !=""){
                        std::string node_name0 = node_variables[node_id]["djrc_name"];
                        std::vector<std::string> v1 = Split(node_name0,';');
                        node_name = v1[0];
                    }
                }
            }

            node_vec.push_back(node_id);
            name_vec.push_back(node_name);
            label1_vec.push_back("NoneLabel");
            label2_vec.push_back("NoneLabel");
            start_vec.push_back("0000-01-01");

            if(node_variables[node_id].find("djrc_adme")!=node_variables[node_id].end()){
                std::vector<std::string> label1_temp = Split(node_variables[node_id]["djrc_adme"],';');
                std::vector<std::string> label2_temp = Split(node_variables[node_id]["djrc_adme2"],';');
                std::vector<std::string> start_temp = Split(node_variables[node_id]["djrc_sourdate"],';');
                if( (label1_temp.size() != label2_temp.size()) || (label2_temp.size() != start_temp.size())){
                    std::cout << "Inconsistent" << std::endl;
                }else{
                    for(int j=0;j<label1_temp.size();++j){
                        node_vec.push_back(node_id);
                        name_vec.push_back(node_name);
                        label1_vec.push_back(label1_temp[j]);
                        label2_vec.push_back(label2_temp[j]);
                        start_vec.push_back(start_temp[j]);
                    }
                }
            }
        }
    }

    void CreateLabelDataFrame2(){
        node_vec.clear();node_vec.shrink_to_fit();
        name_vec.clear();name_vec.shrink_to_fit();
        label1_vec.clear();label1_vec.shrink_to_fit();
        label2_vec.clear();label2_vec.shrink_to_fit();
        start_vec.clear();start_vec.shrink_to_fit();
        for(int i=0;i<core_nodes.size();++i){
            std::string node_id = core_nodes[i];
            std::string node_name = node_variables[node_id]["name"];
            node_vec.push_back(node_id);
            name_vec.push_back(node_name);
            label1_vec.push_back("NoneLabel");
            label2_vec.push_back("NoneLabel");
            start_vec.push_back("0000-01-01");
            if(node_variables[node_id].find("djrc_adme")!=node_variables[node_id].end()){
                std::vector<std::string> label1_temp = Split(node_variables[node_id]["djrc_adme"],';');
                std::vector<std::string> label2_temp = Split(node_variables[node_id]["djrc_adme2"],';');
                std::vector<std::string> start_temp = Split(node_variables[node_id]["djrc_sourdate"],';');
                if( (label1_temp.size() != label2_temp.size()) || (label2_temp.size() != start_temp.size())){
                    std::cout << "Inconsistent" << std::endl;
                }else{
                    for(int j=0;j<label1_temp.size();++j){
                        node_vec.push_back(node_id);
                        name_vec.push_back(node_name);
                        label1_vec.push_back(label1_temp[j]);
                        label2_vec.push_back(label2_temp[j]);
                        start_vec.push_back(start_temp[j]);
                    }
                }
            }
        }
    }



    std::vector<std::string> node_vector2,train_positive_time,test_positive_time;

    void ClearNodeVector(){
        node_vector2.clear();
        node_vector2.shrink_to_fit();
        train_positive_time.clear();
        train_positive_time.shrink_to_fit();
        test_positive_time.clear();
        test_positive_time.shrink_to_fit();
    }

    void AddCountryIndustry(std::string file0){
        std::ofstream ofs(file0);
        std::unordered_map<std::string,int> train_positive_time_dict,test_positive_time_dict;
        for(int i=0;i<train_positive_time.size();++i){
            train_positive_time_dict[train_positive_time[i]] = 1;
        }
        for(int i=0;i<test_positive_time.size();++i){
            test_positive_time_dict[test_positive_time[i]] = 1;
        }

        for(int i=0;i<node_vector2.size();++i){
            std::string node = node_vector2[i];
            std::string country = "unknown";
            std::string industry = "unknown";
            std::string train_label = "0";
            std::string test_label = "0";
            if(node_variables[node].find("djrc_country")!=node_variables[node].end()){
                country = node_variables[node]["djrc_country"];
                std::transform(country.begin(),country.end(),country.begin(),::tolower);
                std::vector<std::string> v1 = Split(country,';');
                country = v1[0];
                if(country == "-") country = "unknown";
            }else if(node_variables[node].find("capitaliq_country")!=node_variables[node].end()){
                country = node_variables[node]["capitaliq_country"];
                std::transform(country.begin(),country.end(),country.begin(),::tolower);
                std::vector<std::string> v1 = Split(country,';');
                country = v1[0];
                if(country == "-") country = "unknown";
            }
            if(node_variables[node].find("capitaliq_industry")!=node_variables[node].end()){
                industry = node_variables[node]["capitaliq_industry"];
                std::transform(industry.begin(),industry.end(),industry.begin(),::tolower);
                std::vector<std::string> v1 = Split(industry,';');
                industry = v1[0];
                if(industry == "-") industry = "unknown";
            }
            if(train_positive_time_dict.find(node)!=train_positive_time_dict.end()){
                train_label = "1";
            }
            if(test_positive_time_dict.find(node)!=test_positive_time_dict.end()){
                test_label = "1";
            }
            ofs << node << "," << country << "," << industry << "," << train_label << "," << test_label << std::endl;
        }
    }

    template<class Archive>
    void serialize(Archive & archive){
        archive(
            node_filelist,node_pathlist,header_file,header_name,
            node_vector,value_vector,variable_name_vector
        );
    }

    // Omake
    static std::string GetClassName(){
      return "Nodes";}
    void SetName(const std::string &input){
        name = input;}
    std::string GetName(){
        return name;}
};

class Edges {// Edges

    // Start Never Erase //
    public:
    std::string name;
    Edges(const std::string &name0){// Constructor
        name = name0;
    }

    // OBJECTS USED THROUGHOUT
    std::unordered_map<std::string,long long> core_nodes_dict;

    // OBJECTS FOR GLOBAL NETWORK: Create SubNetwork
    long long line_counter,global_edge_id_counter,global_num_train,global_num_test;
    std::vector<std::string> global_source_train,global_relation_train,global_target_train,start_train;
    std::unordered_multimap<std::string,std::string> collapsed_edgelist_multimap;
    std::unordered_map<std::string,long long> global_relation2cnt;
    std::vector<std::string> global_relation;
    std::vector<long long> global_relation_cnt;

    void CountLine(){// COUNT LINE IN EL : ONLY RUN ONCE
        std::string str;
        global_edge_id_counter = 0;
        std::ifstream file("/home/rh/Arbeitsraum/Files/KG/All/glo_all_edges_all.txt.gz", std::ios_base::in | std::ios_base::binary);
        try{
            boost::iostreams::filtering_istream in;
            in.push(boost::iostreams::gzip_decompressor());
            in.push(file);
            for(std::string str; std::getline(in,str); ){
                global_edge_id_counter += 1;
                //std::cout << "Counter " << global_edge_id_counter << std::endl;
            }
        }catch(const boost::iostreams::gzip_error& e){
            std::cout << e.what() << std::endl;
        }
        std::cout << "Num Line " << global_edge_id_counter << std::endl;
    }


    long long global_node_id_counter;
    void GlobalRelation2Cnt(){
        std::string str;
        global_node_id_counter = 0;
        global_edge_id_counter = 0;

        std::unordered_map<std::string,long long> global_node_id2cnt;

        std::ifstream file("/home/rh/Arbeitsraum/Files/KG/All/glo_all_edges_all.txt.gz", std::ios_base::in | std::ios_base::binary);
        try{
            boost::iostreams::filtering_istream in;
            in.push(boost::iostreams::gzip_decompressor());
            in.push(file);
            for(std::string str; std::getline(in,str); ){
                std::vector<std::string> elements = Split(str,',');
                std::string source_word = elements[0];
                std::string target_word = elements[2];
                if(global_node_id2cnt.find(source_word)==global_node_id2cnt.end()){
                    global_node_id2cnt[source_word] = global_node_id_counter;
                    global_node_id_counter += 1;
                }
                if(global_node_id2cnt.find(target_word)==global_node_id2cnt.end()){
                    global_node_id2cnt[target_word] = global_node_id_counter;
                    global_node_id_counter += 1;
                }
                std::string relation_word = elements[1];
                if(global_relation2cnt.find(relation_word)==global_relation2cnt.end()){
                    global_relation2cnt[relation_word] = 1;
                }else{
                    global_relation2cnt[relation_word] += 1;
                }
                global_edge_id_counter += 1;
                if(global_edge_id_counter % 10000000 == 0){
                    std::cout << global_edge_id_counter << std::endl;
                }
            }
        }catch(const boost::iostreams::gzip_error& e){
            std::cout << e.what() << std::endl;
        }
        for(auto itr=global_relation2cnt.begin();itr!=global_relation2cnt.end();++itr){
            global_relation.push_back(itr->first);
            global_relation_cnt.push_back(itr->second);
        }
        std::cout << "Number of global node: " << boost::lexical_cast<std::string>(global_node_id_counter) << std::endl;
        std::cout << "Number of global edge: " << boost::lexical_cast<std::string>(global_edge_id_counter) << std::endl;
    }

    void SetEdgeListFromFileGlobal(std::string &train_test_split_time,int &cut_unknown){
        line_counter = 0;
        global_edge_id_counter = 0;
        global_num_train = 0;
        global_num_test = 0;
        std::string source_edge_id,target_edge_id;
        std::unordered_map<std::string,long long> top_relation,nonuse_relation;
        long long skip_line_a = 0;
        long long skip_line_b = 0;

        std::unordered_map<std::string,int> ng_key_person;
        std::ifstream ofsNG("/home/rh/Arbeitsraum/Files/KG/All/serial/0_ng_key_person.txt",std::ios_base::in);
        for(std::string line;std::getline(ofsNG,line);){
            //std::vector<std::string> elements = Split(line,',');
            std::vector<std::string> elements = SplitDqcsv(line);
            elements[0] = Trimquote(elements[0]);
            std::cout << elements[0] << std::endl;
            ng_key_person[elements[0]] = 1;
        }

        std::unordered_map<std::string,int> tuple_string_inclusion;
        std::unordered_map<std::string,std::string> invoice_send,invoice_receive,invoice_contain,invoice_time;
        std::ifstream ofsA("/home/rh/Arbeitsraum/Files/KG/All/serial/Global_Relation_Counter_HOZON.csv",std::ios_base::in);
        for(std::string line;std::getline(ofsA,line);){
            //std::vector<std::string> elements = Split(line,',');
            std::vector<std::string> elements = SplitDqcsv(line);
            if(boost::lexical_cast<long long>(elements[1]) > 99){
              elements[0] = Trimquote(elements[0]);
                top_relation[elements[0]] = boost::lexical_cast<long long>(elements[1]);
            }
        }
        // KOKO DE VERSION WO KAERU
        std::ifstream
        //ofsB("/home/rh/Arbeitsraum/Files/KG/All/serial/nonuse_dbpedia_relation_version2.csv",std::ios_base::in);
        // This was changed to include country information
        ofsB("/home/rh/Arbeitsraum/Files/KG/All/serial/nonuse_dbpedia_relation_version3.csv",std::ios_base::in);
        for(std::string line;std::getline(ofsB,line);){
            std::string tempstring = Trimstring(line);
            nonuse_relation[tempstring] = 1;
        }
        std::ifstream filestream_edgelist("/home/rh/Arbeitsraum/Files/KG/All/glo_all_edges_all.txt.gz",std::ios_base::in | std::ios_base::binary);
        try{
            boost::iostreams::filtering_istream input_stream;
            input_stream.push(boost::iostreams::gzip_decompressor());
            input_stream.push(filestream_edgelist);
            int num_elements = 0;
            for(std::string line;std::getline(input_stream,line);){// Loop through a file
                if(line_counter==0){// Find num_elements using header
                    std::vector<std::string> elements = Split(line,',');
                    num_elements = elements.size();
                }else{
                    std::vector<std::string> elements = Split(line,',');
                    for(int i=0;i<elements.size();++i){
                        elements[i] = Trimquote(elements[i]);}
                    std::string source_word = elements[0];
                    std::string relation_word = elements[1];
                    std::string target_word = elements[2];
                    std::string time_start,time_end;
                    if(num_elements > 5){
                        time_start = elements[3];
                        time_end = elements[4];}

                    if(time_start < train_test_split_time ){
                        int use_relation = 1;// Set to 1
                        if(nonuse_relation.find(relation_word)!=nonuse_relation.end()){
                            use_relation = 0;// inlucded in nonuse_dbpedia so set to 0
                            skip_line_a += 1;}
                        if(top_relation.find(relation_word)==top_relation.end()){
                            use_relation = 0;// relation too small so set to 0
                            skip_line_b += 1;}
                        if(use_relation==1){// Deduplicate name
                            if(relation_word == "capitaliq_Supplier"){
                                relation_word = "supplier";}
                            if(relation_word == "factset_SUPPLIER"){
                                relation_word = "supplier";}
                            if(relation_word == "capitaliq_Customer"){
                                relation_word = "customer";}
                            if(relation_word == "factset_CUSTOMER"){
                                relation_word = "customer";}
                            if(relation_word == "factset_issue"){
                                relation_word = "issue";}
                            if(relation_word == "capitaliq_issue"){
                                relation_word = "issue";}
                            if(relation_word == "reuters_issue"){
                                relation_word = "issue";}
                            if(relation_word == "djrc_homepage"){
                                relation_word = "homepage";}
                            if(relation_word == "reuters_homepage"){
                                relation_word = "homepage";}
                            if(relation_word == "factset_PARTNER-DISTRIB"){
                                relation_word = "distributor";}
                            if(relation_word == "capitaliq_Distributor"){
                                relation_word = "distributor";}

                            // NEW !
                            if(relation_word == "reuters_located_in"){
                                relation_word = "located_in";}
                            if(relation_word == "capitaliq_located_in"){
                                relation_word = "located_in";}
                            if(relation_word == "factship_located_in"){
                                relation_word = "located_in";}
                            if(relation_word == "factset_located_in"){
                                relation_word = "located_in";}
                            if(relation_word == "djrc_located_in"){
                                relation_word = "located_in";}




                            if(relation_word == "factship_from_invoice"){// Invoice Receiver
                                invoice_receive[source_word] = target_word;
                            }else if(relation_word == "factship_to_invoice"){// Invoice Sender
                                invoice_send[target_word] = source_word;
                            }else if(relation_word == "factship_contains"){
                                invoice_contain[source_word] = target_word;
                                invoice_time[source_word] = time_start;
                            }else{
                                if(relation_word=="reuters_owns"){
                                    if(boost::lexical_cast<double>(elements[5])<0.05){
                                        use_relation = 0;}}
                                if(relation_word=="http://dbpedia.org/ontology/keyPerson" && (ng_key_person.find(source_word)!=ng_key_person.end() || ng_key_person.find(target_word)!=ng_key_person.end()) ){
                                    use_relation = 0;
                                    //std::cout << "NG KEY PER" << std::endl;
                                }

                                if(cut_unknown == 1){
                                    if(time_start < "0100-01-01"){
                                        use_relation = 0;
                                    }
                                }
                                if(use_relation == 1){// train
                                    std::string tempstring = source_word + "," + relation_word + "," + target_word;
                                    if(tuple_string_inclusion.find(tempstring)==tuple_string_inclusion.end()){
                                        tuple_string_inclusion[tempstring] = 1;
                                        global_source_train.push_back(source_word);
                                        global_relation_train.push_back(relation_word);
                                        global_target_train.push_back(target_word);
                                        start_train.push_back(time_start);
                                        target_edge_id = target_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                                        collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(source_word,target_edge_id));
                                        source_edge_id = source_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                                        collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(target_word,target_edge_id));
                                        global_num_train += 1;
                                        global_edge_id_counter += 1;
                                    }
                                }
                            }
                        }
                    }// if train_test_split_time
                }
                line_counter += 1;
                if(line_counter % 5000000 == 0){
                    std::cout << "Finished Reading: " << line_counter << std::endl;
                }
                //if(line_counter>30000000){
                if(line_counter>448803947){
                    //break;
                }
            }
        }catch(const boost::iostreams::gzip_error& error){
            std::cout << error.what() << std::endl;
        }
        std::cout << "Calculate FactShip" << std::endl;
        for(auto itr=invoice_send.begin();itr!=invoice_send.end();++itr){
            if(invoice_receive.find(itr->first) != invoice_receive.end() && invoice_contain.find(itr->first) != invoice_contain.end()){
                std::string source_word = itr->second;
                std::string target_word = invoice_receive[itr->first];
                std::string relation_word = "international_shipping";
                std::string tempstring = source_word + "," + relation_word + "," + target_word;
                std::string time_start = invoice_time[itr->first];
                if(tuple_string_inclusion.find(tempstring)==tuple_string_inclusion.end()){
                    tuple_string_inclusion[tempstring] = 1;
                    global_source_train.push_back(source_word);
                    global_relation_train.push_back(relation_word);
                    global_target_train.push_back(target_word);
                    start_train.push_back(time_start);
                    target_edge_id = target_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    // uses WORD
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(source_word,target_edge_id));
                    source_edge_id = source_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    // uses WORD
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(target_word,target_edge_id));
                    global_num_train += 1;
                    global_edge_id_counter += 1;
                }
                // Send goods
                source_word = itr->second;
                target_word = invoice_contain[itr->first];
                relation_word = "send_goods";
                tempstring = source_word + "," + relation_word + "," + target_word;
                if(tuple_string_inclusion.find(tempstring)==tuple_string_inclusion.end()){
                    tuple_string_inclusion[tempstring] = 1;
                    global_source_train.push_back(source_word);
                    global_relation_train.push_back(relation_word);
                    global_target_train.push_back(target_word);
                    start_train.push_back(time_start);
                    target_edge_id = target_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(source_word,target_edge_id));
                    source_edge_id = source_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(target_word,target_edge_id));
                    global_num_train += 1;
                    global_edge_id_counter += 1;
                }
                // Receive
                source_word = invoice_receive[itr->first];
                target_word = invoice_contain[itr->first];
                relation_word = "recieve_goods";
                tempstring = source_word + "," + relation_word + "," + target_word;
                if(tuple_string_inclusion.find(tempstring)==tuple_string_inclusion.end()){
                    tuple_string_inclusion[tempstring] = 1;
                    global_source_train.push_back(source_word);
                    global_relation_train.push_back(relation_word);
                    global_target_train.push_back(target_word);
                    start_train.push_back(time_start);
                    target_edge_id = target_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(source_word,target_edge_id));
                    source_edge_id = source_word + "," + boost::lexical_cast<std::string>(global_edge_id_counter);
                    collapsed_edgelist_multimap.insert(std::pair<std::string,std::string>(target_word,target_edge_id));
                    global_num_train += 1;
                    global_edge_id_counter += 1;
                }
            }
        }
        std::cout << "Num Train " << global_num_train << std::endl;
        std::cout << "Skip Line A " << skip_line_a << std::endl;
        std::cout << "Skip Line B " << skip_line_b << std::endl;
        //std::cout << "Num Test " << global_num_test << std::endl;
    }// End SetEdgeListFromFile

    void CreateSubNetwork(int &depth,std::string &output_file){

        std::string source_node,target_node;
        std::unordered_map<std::string,int> node_visited0,node_visited1,node_visited2,node_visited3,node_visited4,node_visited5;
        std::unordered_map<long long,int> global_edge_id_inclusion;
        std::ofstream ofs(output_file);
        int i = 0;
        for(auto itr=core_nodes_dict.begin();itr!=core_nodes_dict.end();++itr){
            // Our starting node
            source_node = itr->first;
            node_visited0[source_node] = 1;// KOKO
            std::vector<std::string> step1,step2,step3,step4;
            if(depth>0){// 1 step
                auto range = collapsed_edgelist_multimap.equal_range(source_node);
                for(auto itr=range.first;itr!=range.second;++itr){
                    std::string node_b_edge_id = itr->second;
                    std::vector<std::string> elements = Split(node_b_edge_id,',');
                    target_node = elements[0];// 0 is node info
                    if(node_visited1.find(target_node) == node_visited1.end()){// Add to next step
                        step1.push_back(target_node);// KOKO
                        node_visited1[target_node] = 1;// KOKO
                    }
                    long long global_edge_id = boost::lexical_cast<long long>(elements[1]);// 1 is global_edge_id
                    if(global_edge_id_inclusion.find(global_edge_id)==global_edge_id_inclusion.end()){
                        global_edge_id_inclusion[global_edge_id] = 1;
                        ofs << global_source_train[global_edge_id] << "," << global_relation_train[global_edge_id] << "," << global_target_train[global_edge_id] << "," << start_train[global_edge_id] << std::endl;}}
            }// End 1step

            if(depth>1){// 2step
                for(int j=0;j<step1.size();++j){
                    source_node = step1[j];
                    auto range2 = collapsed_edgelist_multimap.equal_range(source_node);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        std::string node_b_edge_id = itr2->second;
                        std::vector<std::string> elements = Split(node_b_edge_id,',');
                        target_node = elements[0];// 0 is node info
                        if( node_visited0.find(target_node) == node_visited0.end() ){
                            if( node_visited1.find(target_node) == node_visited1.end() &&
                            node_visited2.find(target_node) == node_visited2.end()){
                                step2.push_back(target_node);// KOKO
                                node_visited2[target_node] = 1;// KOKO
                            }
                            long long global_edge_id = boost::lexical_cast<long long>(elements[1]);
                            if(global_edge_id_inclusion.find(global_edge_id)==global_edge_id_inclusion.end()){
                                  global_edge_id_inclusion[global_edge_id] = 1;
                                  ofs << global_source_train[global_edge_id] << "," << global_relation_train[global_edge_id] << "," << global_target_train[global_edge_id] << "," << start_train[global_edge_id] << std::endl;}}}}
            }// End 2step

            if(depth>2){// 3step
                for(int j=0;j<step2.size();++j){
                    source_node = step2[j];
                    auto range2 = collapsed_edgelist_multimap.equal_range(source_node);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        std::string node_b_edge_id = itr2->second;
                        std::vector<std::string> elements = Split(node_b_edge_id,',');
                        target_node = elements[0];// 0 is node info
                        if( node_visited0.find(target_node) == node_visited0.end() && node_visited1.find(target_node) == node_visited1.end() ){
                            if(node_visited2.find(target_node) == node_visited2.end() && node_visited3.find(target_node) == node_visited3.end()){
                                step3.push_back(target_node);// KOKO
                                node_visited3[target_node] = 1;// KOKO
                            }
                            long long global_edge_id = boost::lexical_cast<long long>(elements[1]);
                            if(global_edge_id_inclusion.find(global_edge_id)==global_edge_id_inclusion.end()){
                                  global_edge_id_inclusion[global_edge_id] = 1;
                                  ofs << global_source_train[global_edge_id] << "," << global_relation_train[global_edge_id] << "," << global_target_train[global_edge_id] << "," << start_train[global_edge_id] << std::endl;}}}}
            }// End 3step

            if(depth>3){// 4step
                for(int j=0;j<step3.size();++j){
                    source_node = step3[j];
                    auto range2 = collapsed_edgelist_multimap.equal_range(source_node);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        std::string node_b_edge_id = itr2->second;
                        std::vector<std::string> elements = Split(node_b_edge_id,',');
                        target_node = elements[0];// 0 is node info
                        if( node_visited0.find(target_node) == node_visited0.end() && node_visited1.find(target_node) == node_visited1.end() && node_visited2.find(target_node) == node_visited2.end()){
                            if( node_visited3.find(target_node) == node_visited3.end() && node_visited4.find(target_node) == node_visited4.end()){
                                step4.push_back(target_node);// KOKO
                                node_visited4[target_node] = 1;// KOKO
                            }
                            long long global_edge_id = boost::lexical_cast<long long>(elements[1]);
                            if(global_edge_id_inclusion.find(global_edge_id)==global_edge_id_inclusion.end()){
                                global_edge_id_inclusion[global_edge_id] = 1;
                                ofs << global_source_train[global_edge_id] << "," << global_relation_train[global_edge_id] << "," << global_target_train[global_edge_id] << "," << start_train[global_edge_id] << std::endl;}}}}
            }// End 4step

            if(depth>4){// 5step
                for(int j=0;j<step4.size();++j){
                    source_node = step4[j];
                    auto range2 = collapsed_edgelist_multimap.equal_range(source_node);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        std::string node_b_edge_id = itr2->second;
                        std::vector<std::string> elements = Split(node_b_edge_id,',');
                        target_node = elements[0];// 0 is Node Info
                        if( node_visited0.find(target_node) == node_visited0.end() && node_visited1.find(target_node) == node_visited1.end() && node_visited2.find(target_node) == node_visited2.end() && node_visited3.find(target_node) == node_visited3.end()){
                            if(node_visited4.find(target_node) == node_visited4.end() && node_visited5.find(target_node) == node_visited5.end()){
                                //step5.push_back(target_node);// KOKO
                                node_visited5[target_node] = 1;// KOKO
                            }
                            long long global_edge_id = boost::lexical_cast<long long>(elements[1]);
                            if(global_edge_id_inclusion.find(global_edge_id)==global_edge_id_inclusion.end()){
                                  global_edge_id_inclusion[global_edge_id] = 1;
                                  ofs << global_source_train[global_edge_id] << "," << global_relation_train[global_edge_id] << "," << global_target_train[global_edge_id] << "," << start_train[global_edge_id] << std::endl;}}}}
            }// End 5step
            if(i % 1 == 0){
                std::cout << "Finished Crowling: " << i << std::endl;
                std::cout << "Step Size: " << step1.size() << "," << step2.size() << "," << step3.size() << "," <<  step4.size() << std::endl;
            }
            i += 1;
        }
        std::cout << "Out of Loop" << std::endl;
    }

    // OBJECTS FOR SUB NETWORK
    std::vector<std::string> files_edgelist;
    long long node_id_counter = 0;
    long long relation_id_counter = 0;
    long long edge_id_counter = 0;
    long long edge_id_2step_counter = 0;
    long long collapsed_edge_id_counter = 0;
    long long collapsed_edge_id_2step_counter = 0;
    long long core_node_id_counter = 0;
    std::unordered_map<long long,long long> core_id2id,id2core_id;
    std::vector<long long> source_train,relation_train,target_train;
    std::vector<int> source_2path_train,target_2path_train;
    std::unordered_map<std::string,long long> node2id,relation2id,relation2cnt;
    std::vector<std::string> relation;
    std::vector<long long> relation_cnt;
    std::unordered_map<long long,std::string> id2node,id2relation;
    std::unordered_map<std::string,long long> edge_dict_train;
    std::unordered_map<double,int> pair_indicator_train;
    std::unordered_multimap<long long,long long> edgelist_collapsed_multimap_train;
    std::unordered_multimap<long long,std::string> source_multimap_train,target_multimap_train;
    std::unordered_multimap<std::string,std::string> pair_multimap_train;
    std::unordered_map<long long,std::vector<long long>> relation_edge;
    std::unordered_map<long long,long long> collapsed_edge_id2edge_id;
    std::unordered_map<double,long long> pair2collapsed_edge_id;
    std::unordered_map<long long,std::vector<long long>> edge_parallel,edge_source,edge_target;
    std::unordered_map<long long,std::unordered_map<int,std::unordered_map<int,std::vector<long long>>>> edge_depth_order_backpath;
    std::unordered_map<long long,std::unordered_map<int,std::unordered_map<int,std::vector<long long>>>> collapsed_edge_depth_order_backpath;
    std::unordered_map<long long,std::unordered_map<int,std::unordered_map<std::string,std::vector<std::string>>>> edge_depth_path_relation;
    std::unordered_map<long long,std::unordered_map<int,std::unordered_map<std::string,std::vector<std::string>>>> collapsed_edge_depth_path_relation;
    std::vector<long long> source_backpath,relation_backpath,target_backpath,edge_id_backpath;
    std::unordered_multimap<long long,std::string> source_multimap_backpath,target_multimap_backpath;
    Eigen::MatrixXf edge_matrix,topic_matrix;
    Eigen::SparseMatrix<float, Eigen::ColMajor,long long> sparse_core_matrix,sparse_core_matrix_temp;
    Eigen::SparseMatrix<float, Eigen::RowMajor,long long> p_matrix;
    std::unordered_map<int,std::unordered_map<int,Eigen::MatrixXf>> depth_order_weight;
    Eigen::MatrixXf output_weight;
    std::unordered_map<int,std::unordered_map<int,Eigen::MatrixXf>> depth_order_derivative_accu;
    Eigen::MatrixXf output_derivative_accu;
    std::unordered_map<long long,int> collapsed_edge_id_is_core;

    // edge_id is added at the end
    void UpdateMultimap(std::unordered_multimap<long long,std::string> &source_multimap0,std::unordered_multimap<long long,std::string> &target_multimap0,long long source_id,long long relation_id,long long target_id,long long edge_id){
        // update source_multimap
        std::string target_relation = boost::lexical_cast<std::string>(target_id) + "," + boost::lexical_cast<std::string>(relation_id) + "," + boost::lexical_cast<std::string>(edge_id);
        source_multimap0.insert(std::pair<long long,std::string>(source_id,target_relation));
        // update target_multimap
        std::string source_relation = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(relation_id) + "," + boost::lexical_cast<std::string>(edge_id);
        target_multimap0.insert(std::pair<long long,std::string>(target_id,source_relation));
    }// End of UpdateMultiMap

    void SetEdgeListFromFile(std::string &train_test_split_time){// Set files_edgelist before use

        collapsed_edge_id_counter = 1;

        for(int i=0;i<files_edgelist.size();++i){// Loop over files
            std::ifstream filestream_edgelist(files_edgelist[i],std::ios_base::in | std::ios_base::binary);
            try{
                boost::iostreams::filtering_istream input_stream;
                input_stream.push(boost::iostreams::gzip_decompressor());
                input_stream.push(filestream_edgelist);
                long long line_counter = 0;
                int num_elements = 0;
                for(std::string line;std::getline(input_stream,line);){// Loop through a file
                    if(line_counter==0){// Find num_elements using header
                        std::vector<std::string> elements = Split(line,',');
                        num_elements = elements.size();
                    }else{
                        std::vector<std::string> elements = Split(line,',');
                        for(int i=0;i<elements.size();++i){
                            elements[i] = Trimquote(elements[i]);
                        }
                        std::string source_word = elements[0];
                        std::string relation_word = elements[1];
                        std::string target_word = elements[2];
                        std::string time_start,time_end;
                        if(num_elements > 3){
                            time_start = elements[3];
                        }

                        // time_train.push_back(time_start);

                        if(time_start <= train_test_split_time){// Train
                            // Create node<->id dict
                            if(node2id.find(source_word) == node2id.end()){
                                node2id[source_word] = node_id_counter;
                                id2node[node_id_counter] = source_word;
                                node_id_counter += 1;
                            }
                            if(node2id.find(target_word) == node2id.end()){
                                node2id[target_word] = node_id_counter;
                                id2node[node_id_counter] = target_word;
                                node_id_counter += 1;
                            }
                            if(relation2id.find(relation_word) == relation2id.end()){
                                relation2id[relation_word] = relation_id_counter;
                                id2relation[relation_id_counter] = relation_word;
                                relation_id_counter += 1;
                            }
                            // Transform to ID
                            long long source_id = node2id.at(source_word);
                            long long relation_id = relation2id.at(relation_word);
                            long long target_id = node2id.at(target_word);

                            // edge_dict_train : What about using CantorPair?
                            std::string source_relation_target = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(relation_id) + ","+ boost::lexical_cast<std::string>(target_id);

                            if(edge_dict_train.find(source_relation_target)==edge_dict_train.end()){
                                edge_dict_train[source_relation_target] = edge_id_counter;
                                // Create Edgelist Collpased Version
                                double pair1 = ContorPair(double(source_id),double(target_id));
                                double pair2 = ContorPair(double(target_id),double(source_id));
                                if(pair_indicator_train.find(pair1)==pair_indicator_train.end()){
                                    pair_indicator_train[pair1] = 1;
                                    pair_indicator_train[pair2] = 1;
                                    edgelist_collapsed_multimap_train.insert(std::pair<long long,long long>(source_id,target_id));
                                    edgelist_collapsed_multimap_train.insert(std::pair<long long,long long>(target_id,source_id));
                                    collapsed_edge_id2edge_id[collapsed_edge_id_counter] = edge_id_counter;
                                    pair2collapsed_edge_id[pair1] = collapsed_edge_id_counter;
                                    pair2collapsed_edge_id[pair2] = collapsed_edge_id_counter;
                                    if(core_nodes_dict.find(source_word)!=core_nodes_dict.end() && core_nodes_dict.find(target_word)!=core_nodes_dict.end()){
                                        collapsed_edge_id_is_core[collapsed_edge_id_counter] = 1;
                                    }
                                    collapsed_edge_id_counter += 1;
                                }
                                if(relation2cnt.find(relation_word)==relation2cnt.end()){
                                    relation2cnt[relation_word] = 1;
                                }else{
                                    relation2cnt[relation_word] += 1;}

                                // Update Vectors
                                source_train.push_back(source_id);
                                relation_train.push_back(relation_id);
                                target_train.push_back(target_id);

                                // Insert Tuple to Multimaps
                                UpdateMultimap(source_multimap_train,target_multimap_train,source_id,relation_id,target_id,edge_id_counter);

                                // Pair Multimap : used to calculate BackPath faster
                                std::string source_target_id = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(target_id);

                                std::string relation_edge_id = boost::lexical_cast<std::string>(relation_id) + "," + boost::lexical_cast<std::string>(edge_id_counter);

                                pair_multimap_train.insert(std::pair<std::string,std::string>(source_target_id,relation_edge_id));

                                // relation_id 2 edge_id vector
                                if(relation_edge.find(relation_id)==relation_edge.end()){
                                    std::vector<long long> temp_vector;
                                    temp_vector.push_back(edge_id_counter);
                                    relation_edge[relation_id] = temp_vector;
                                }else{
                                    relation_edge[relation_id].push_back(edge_id_counter);
                                }
                                // iterate edgeid_counter
                                edge_id_counter += 1;
                            }
                        }else{// if not train
                        }
                    }// if not first line
                    line_counter += 1;
                    if(line_counter % 500000 == 0){
                        std::cout << "SetEdgeListFromFile: " << line_counter << std::endl;
                    }
                }//for getline(input_stream, line)
            }catch(const boost::iostreams::gzip_error& error){
                std::cout << error.what() << std::endl;}
              // Confirm what we have read
            std::cout << files_edgelist[i] << std::endl;// End for files_edgelist
        }
        for(auto itr=relation2cnt.begin();itr!=relation2cnt.end();++itr){
            relation.push_back(itr->first);
            relation_cnt.push_back(itr->second);
        }
        std::cout << "Num Nodes: " << boost::lexical_cast<std::string>(node_id_counter) << std::endl;
        std::cout << "Num Edges: " << boost::lexical_cast<std::string>(edge_id_counter) << std::endl;
    }// End SetEdgeListFromFile

    std::vector<std::string> time_train;
    std::unordered_map<std::string,int> edge_dict_train_time;
    long long edge_id_counter_temp = 0;

    void RecoverEdgeTime(std::string &train_test_split_time){// Set files_edgelist before use

        for(int i=0;i<files_edgelist.size();++i){// Loop over files
            std::ifstream filestream_edgelist(files_edgelist[i],std::ios_base::in | std::ios_base::binary);
            try{
                boost::iostreams::filtering_istream input_stream;
                input_stream.push(boost::iostreams::gzip_decompressor());
                input_stream.push(filestream_edgelist);
                long long line_counter = 0;
                int num_elements = 0;
                for(std::string line;std::getline(input_stream,line);){// Loop through a file
                    if(line_counter==0){// Find num_elements using header
                        std::vector<std::string> elements = Split(line,',');
                        num_elements = elements.size();
                    }else{
                        std::vector<std::string> elements = Split(line,',');
                        for(int i=0;i<elements.size();++i){
                            elements[i] = Trimquote(elements[i]);
                        }
                        std::string source_word = elements[0];
                        std::string relation_word = elements[1];
                        std::string target_word = elements[2];
                        std::string time_start,time_end;
                        if(num_elements > 3){
                            time_start = elements[3];
                        }

                        if(time_start <= train_test_split_time){// Train

                            // Transform to ID
                            long long source_id = node2id.at(source_word);
                            long long relation_id = relation2id.at(relation_word);
                            long long target_id = node2id.at(target_word);

                            // edge_dict_train : What about using CantorPair?
                            std::string source_relation_target = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(relation_id) + ","+ boost::lexical_cast<std::string>(target_id);

                            if(edge_dict_train_time.find(source_relation_target)==edge_dict_train_time.end()){
                                edge_dict_train_time[source_relation_target] = edge_id_counter_temp;
                                // Update Vectors
                                time_train.push_back(time_start);
                                // iterate edgeid_counter
                                edge_id_counter_temp += 1;
                            }
                        }else{// if not train
                        }
                    }// if not first line
                    line_counter += 1;
                    if(line_counter % 500000 == 0){
                        std::cout << "SetEdgeListFromFile: " << line_counter << std::endl;
                    }
                }//for getline(input_stream, line)
            }catch(const boost::iostreams::gzip_error& error){
                std::cout << error.what() << std::endl;}
              // Confirm what we have read
            std::cout << files_edgelist[i] << std::endl;// End for files_edgelist
        }
    }// End RecoverEdgeTime

    std::vector<double> degree;
    std::vector<std::string> node_degree;
    std::vector<std::string> core_source,core_target,core_time;

    void DegreeDistribution(int &core_only){
        node_degree.clear();node_degree.shrink_to_fit();
        degree.clear();degree.shrink_to_fit();
        for(auto itr=core_nodes_dict.begin();itr!=core_nodes_dict.end();++itr){
            std::string node_word = itr->first;

            if(node2id.find(node_word)!=node2id.end()){
                long long node_id = node2id.at(node_word);
                double temp_counter = 0;
                auto range_source = source_multimap_train.equal_range(node_id);
                for(auto itr2=range_source.first;itr2!=range_source.second;++itr2){

                    std::vector<std::string> elements = Split(itr2->second,',');
                    long long target_id = boost::lexical_cast<long long>(elements[0]);
                    std::string target_word = id2node[target_id];

                    if(core_only==1){
                        if(core_nodes_dict.find(target_word)!=core_nodes_dict.end()){
                            temp_counter += 1;

                            core_source.push_back(node_word);
                            core_target.push_back(target_word);

                            // edge_dict_train : What about using CantorPair?
                            std::string source_relation_target = boost::lexical_cast<std::string>(node_id) + "," + elements[1] + ","+ boost::lexical_cast<std::string>(target_id);
                            long long edge_id = edge_dict_train_time[source_relation_target];

                            core_time.push_back(time_train[edge_id]);


                        }
                    }else{
                        temp_counter += 1;
                    }
                }
                auto range_target = target_multimap_train.equal_range(node_id);
                for(auto itr2=range_target.first;itr2!=range_target.second;++itr2){
                    std::vector<std::string> elements = Split(itr2->second,',');
                    long long source_id = boost::lexical_cast<long long>(elements[0]);
                    std::string source_word = id2node[source_id];
                    if(core_only==1){
                        if(core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                            temp_counter += 1;
                        }
                    }else{
                        temp_counter += 1;
                    }
                }
                node_degree.push_back(node_word);
                degree.push_back(temp_counter);
            }
        }
    }

    void CreateEdgeid2Edgeid(){// Returns unordered_map<int,std::vector<int>>: edge_parallel/source/target
        for(long long edge_id=0;edge_id<source_train.size();++edge_id){

            if(edge_id % 50000 == 0){
                std::cout << "CreateEdgeid2Edgeid " << edge_id << " Out of " << source_train.size() << std::endl;
            }

            long long source_id = source_train[edge_id];
            long long relation_id = relation_train[edge_id];
            long long target_id = target_train[edge_id];
            std::vector<long long> edge_parallel_vector,edge_source_vector,edge_target_vector;

            // source source
            auto range_source_source = source_multimap_train.equal_range(source_id);
            for(auto itr=range_source_source.first;itr!=range_source_source.second;++itr){
                std::vector<std::string> elements = Split(itr->second,',');
                std::string source_relation_target = boost::lexical_cast<std::string>(source_id) + "," + elements[1] + "," + elements[0];
                if(boost::lexical_cast<std::string>(relation_id) != elements[1] && boost::lexical_cast<std::string>(target_id) == elements[0]){// Parallel Edges
                    edge_parallel_vector.push_back(edge_dict_train[source_relation_target]);}
                if(boost::lexical_cast<std::string>(target_id) != elements[0]){// Source Edges
                    edge_source_vector.push_back(edge_dict_train[source_relation_target]);}}

            // target target
            auto range_target_target = target_multimap_train.equal_range(target_id);
            for(auto itr=range_target_target.first;itr!=range_target_target.second;++itr){
                std::vector<std::string> elements = Split(itr->second,',');
                std::string source_relation_target = elements[0] + "," + elements[1] + "," + boost::lexical_cast<std::string>(target_id);
                if(boost::lexical_cast<std::string>(source_id)!=elements[0]){// Target Edges
                    edge_target_vector.push_back(edge_dict_train[source_relation_target]);}}

            // source target
            auto range_source_target = source_multimap_train.equal_range(target_id);
            for(auto itr=range_source_target.first;itr!=range_source_target.second;++itr){
                std::vector<std::string> elements = Split(itr->second,',');
                std::string source_relation_target = boost::lexical_cast<std::string>(target_id) + "," + elements[1] + "," + elements[0];
                edge_target_vector.push_back(edge_dict_train[source_relation_target]);}

            // target source
            auto range_target_source = target_multimap_train.equal_range(source_id);
            for(auto itr=range_target_source.first;itr!=range_target_source.second;++itr){
                std::vector<std::string> elements = Split(itr->second,',');
                std::string source_relation_target = elements[0] + "," + elements[1] + "," + boost::lexical_cast<std::string>(source_id);
                edge_target_vector.push_back(edge_dict_train[source_relation_target]);}
            // Final Target
            edge_parallel[edge_id] = edge_parallel_vector;
            edge_source[edge_id] = edge_source_vector;
            edge_target[edge_id] = edge_target_vector;
        }
    }

    void UpdateVectorBackPath(long long source_id,long long target_id,std::unordered_map<double,int> &pairs_included,std::vector<long long> &source_temp,std::vector<long long> &target_temp,std::vector<int> &order_temp,int order,std::unordered_map<long long,int> &nodes_visited,int minimum,int add_source_or_target){// Use to Grow BackPath Net
        double pair1 = ContorPair(double(source_id),double(target_id));
        double pair2 = ContorPair(double(target_id),double(source_id));
        if(pairs_included.find(pair1)==pairs_included.end() && pairs_included.find(pair2)==pairs_included.end()){
            pairs_included[pair1] = 1;
            pairs_included[pair2] = 1;
            source_temp.push_back(source_id);
            target_temp.push_back(target_id);
            order_temp.push_back(order);
            if(minimum==1){
                if(add_source_or_target==0){// 0 : source, 1 : target
                    if(nodes_visited.find(source_id)==nodes_visited.end()){
                        nodes_visited[source_id] = 1;}
                }else{
                    if(nodes_visited.find(target_id)==nodes_visited.end()){
                        nodes_visited[target_id] = 1;}}}
        }else{//std::cout << "Efficiency!" << std::endl;
        }
    }// End UpdateVectorBackPath

    std::vector<long long> ReturnRelationType(long long source_id,long long target_id){
        std::vector<long long> relation_type_vector;
        // source -> target direction
        std::string pair1 = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(target_id);
        auto range_pair = pair_multimap_train.equal_range(pair1);
        for(auto itr=range_pair.first;itr!=range_pair.second;++itr){
            std::vector<long long> relation_edge_vector = Split2LongVector(itr->second);
            // FOR GET IT LET IT COLLAPSE
            relation_type_vector.push_back(relation_edge_vector[0]);
            // BE CAREFUL WE ADDED ONE TO RELATION ID
            //relation_type_vector.push_back(relation_edge_vector[0]+1);
        }// LOOP range_pair
        // source <- target direction
        std::string pair2 = boost::lexical_cast<std::string>(target_id) + "," + boost::lexical_cast<std::string>(source_id);
        auto range_pair2 = pair_multimap_train.equal_range(pair2);
        for(auto itr=range_pair2.first;itr!=range_pair2.second;++itr){
            std::vector<long long> relation_edge_vector = Split2LongVector(itr->second);
            // FOR GET IT LET IT COLLAPSE
            relation_type_vector.push_back(relation_edge_vector[0]);
            // BE CAREFUL WE ADDED ONE TO RELATION ID
            //relation_type_vector.push_back(-relation_edge_vector[0]-1);
        }
        return(relation_type_vector);
    }

    // Recover Relation Type and Direction
    void RecoverRelationType(long long edge_id,std::vector<long long> &source_temp,std::vector<long long> &target_temp,std::vector<int> &order_temp,std::unordered_map<double,int> &tuple_indicator,int depth,
    std::unordered_map<int,std::unordered_map<int,std::vector<long long>>> &depth_order_vector){

        // UPDATES : edge_depth_order_backpath

        std::vector<long long> edge_id_vector;
        //std::unordered_map<int,std::vector<long long>> order_vector;
        for(int i=0;i<source_temp.size();++i){
            // source -> target direction
            std::string pair1 = boost::lexical_cast<std::string>(source_temp[i]) + "," + boost::lexical_cast<std::string>(target_temp[i]);
            auto range_pair = pair_multimap_train.equal_range(pair1);
            for(auto itr=range_pair.first;itr!=range_pair.second;++itr){
                std::vector<long long> relation_edge_vector = Split2LongVector(itr->second);
                double unique_value = ContorTuple(double(source_temp[i]),double(relation_edge_vector[0]),double(target_temp[i]));
                if(tuple_indicator.find(unique_value)==tuple_indicator.end()){
                    tuple_indicator[unique_value] = 1;
                    source_backpath.push_back(source_temp[i]);
                    relation_backpath.push_back(relation_edge_vector[0]);
                    target_backpath.push_back(target_temp[i]);
                    long long edge_id_temp = boost::lexical_cast<long long>(source_backpath.size()-1);
                    // Insert Tuple to Multimaps
                    UpdateMultimap(source_multimap_backpath,target_multimap_backpath,source_temp[i],relation_edge_vector[0],target_temp[i],edge_id_temp);
                    edge_depth_order_backpath[edge_id][depth][order_temp[i]].push_back(relation_edge_vector[1]);
                }
            }// LOOP range_pair

            // source <- target direction
            std::string pair2 = boost::lexical_cast<std::string>(target_temp[i]) + "," + boost::lexical_cast<std::string>(source_temp[i]);
            auto range_pair2 = pair_multimap_train.equal_range(pair2);
            for(auto itr=range_pair2.first;itr!=range_pair2.second;++itr){
                std::vector<long long> relation_edge_vector = Split2LongVector(itr->second);
                double unique_value = ContorTuple(double(target_temp[i]),double(relation_edge_vector[0]),double(source_temp[i]));
                if(tuple_indicator.find(unique_value)==tuple_indicator.end()){
                    tuple_indicator[unique_value] = 1;
                    source_backpath.push_back(target_temp[i]);
                    relation_backpath.push_back(relation_edge_vector[0]);
                    target_backpath.push_back(source_temp[i]);
                    long long edge_id_temp = boost::lexical_cast<long long>(source_backpath.size()-1);
                    UpdateMultimap(source_multimap_backpath,target_multimap_backpath,target_temp[i],relation_edge_vector[0],source_temp[i],edge_id_temp);
                    edge_depth_order_backpath[edge_id][depth][order_temp[i]].push_back(relation_edge_vector[1]);
                }
            }
        }// End of for(source_temp)
    }// End of RecoverRelationType

    void BackPath(long long source_id,long long relation_id,long long target_id,int depth,int minimum){

        // UPDATES : edge_depth_order_backpath

        // OK : edge_id
        std::string source_relation_target = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(relation_id) + "," + boost::lexical_cast<std::string>(target_id);
        long long edge_id = edge_dict_train[source_relation_target];// edge_id with relation and direction
        // Initialize
        std::unordered_map<int,std::unordered_map<int,std::vector<long long>>> depth_order_vector;
        source_backpath.clear();source_backpath.shrink_to_fit();
        relation_backpath.clear();relation_backpath.shrink_to_fit();
        target_backpath.clear();target_backpath.shrink_to_fit();
        std::unordered_map<long long,int> nodes_visited;
        nodes_visited[source_id] = 1;
        nodes_visited[target_id] = 1;
        // Avoid double counting : collapsed edgelist
        std::unordered_map<double,int> pairs_included;// Re Initialize
        double pair1 = ContorPair(double(source_id),double(target_id));
        double pair2 = ContorPair(double(target_id),double(source_id));
        pairs_included[pair1] = 1;
        pairs_included[pair2] = 1;

        // Avoid double counting : tuple list
        std::unordered_map<double,int> tuple_indicator;
        double unique_value1 = ContorTuple(double(source_id),double(relation_id),double(target_id));
        if(tuple_indicator.find(unique_value1)==tuple_indicator.end()){
            tuple_indicator[unique_value1] = 1;
            source_backpath.push_back(source_id);
            relation_backpath.push_back(relation_id);
            target_backpath.push_back(target_id);
            UpdateMultimap(source_multimap_backpath,target_multimap_backpath,
                    source_id,relation_id,target_id,int(source_backpath.size())-1);}

        if(depth >= 1){
            std::vector<long long> source_temp,target_temp;
            std::vector<int> order_temp;
            source_temp.push_back(source_id);
            target_temp.push_back(target_id);
            order_temp.push_back(1);
            RecoverRelationType(edge_id,source_temp,target_temp,order_temp,tuple_indicator,1,depth_order_vector);

            // ADDED
            std::string path = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(target_id);
            std::vector<long long> relation_type = ReturnRelationType(source_id,target_id);
            std::vector<std::string> relation_type_string;
            for(int ii=0;ii<relation_type.size();++ii){
                relation_type_string.push_back(boost::lexical_cast<std::string>(relation_type[ii]));
            }
            edge_depth_path_relation[edge_id][1][path] = relation_type_string;

        }// End of depth >= 1

        if(depth >= 2){
            std::unordered_map<long long,int> nodes_visited_lagged = nodes_visited;
            // This version uses succint collapsed edgelist
            std::vector<long long> source_temp,target_temp;
            std::vector<int> order_temp;
            auto range = edgelist_collapsed_multimap_train.equal_range(source_id);
            for(auto itr=range.first;itr!=range.second;++itr){
                if(nodes_visited_lagged.find(itr->second)==nodes_visited_lagged.end()){
                    unique_value1 = ContorPair(double(itr->second),double(target_id));
                    if(pair_indicator_train.find(unique_value1)!=pair_indicator_train.end()){
                        // source <-> node1 :
                        UpdateVectorBackPath(source_id,itr->second,pairs_included,
                        source_temp,target_temp,order_temp,1,nodes_visited,minimum,1);
                        // node1 <-> target
                        UpdateVectorBackPath(itr->second,target_id,pairs_included,
                        source_temp,target_temp,order_temp,2,nodes_visited,minimum,0);
                        // ADDED
                        std::string path = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(itr->second) + "," +  boost::lexical_cast<std::string>(target_id);
                        std::vector<long long> relation_type1 = ReturnRelationType(source_id,itr->second);
                        std::vector<long long> relation_type2 = ReturnRelationType(itr->second,target_id);
                        std::vector<std::string> relation_type_string;
                        for(int ii=0;ii<relation_type1.size();++ii){
                            for(int jj=0;jj<relation_type2.size();++jj){
                                std::string temp_string = boost::lexical_cast<std::string>(relation_type1[ii]) + "," + boost::lexical_cast<std::string>(relation_type2[jj]);
                                relation_type_string.push_back(temp_string);
                            }
                        }
                        edge_depth_path_relation[edge_id][2][path] = relation_type_string;
                    }
                }
            }
            RecoverRelationType(edge_id,source_temp,target_temp,order_temp,tuple_indicator,2,depth_order_vector);
        }// end of depth >= 2

        if(depth >= 3){
            std::unordered_map<long long,int> nodes_visited_lagged = nodes_visited;
            // This version uses succint collapsed edgelist
            std::vector<long long> source_temp,target_temp;
            std::vector<int> order_temp;
            auto range = edgelist_collapsed_multimap_train.equal_range(source_id);
            auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
            for(auto itr=range.first;itr!=range.second;++itr){
                if(nodes_visited_lagged.find(itr->second)==nodes_visited_lagged.end()){
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        if(nodes_visited_lagged.find(itr2->second)==nodes_visited_lagged.end()){
                            double unique_value1 = ContorPair(double(itr->second),double(itr2->second));
                            if(pair_indicator_train.find(unique_value1)!=pair_indicator_train.end()){
                                // source <-> node1
                                UpdateVectorBackPath(source_id,itr->second,pairs_included,source_temp,target_temp,order_temp,1,nodes_visited,minimum,1);
                                // node1 <-> node2
                                UpdateVectorBackPath(itr->second,itr2->second,pairs_included,source_temp,target_temp,order_temp,2,nodes_visited,minimum,1);
                                // node2 <-> target
                                UpdateVectorBackPath(itr2->second,target_id,pairs_included,source_temp,target_temp,order_temp,3,nodes_visited,minimum,0);

                                // ADDED
                                std::string path = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(itr->second) + "," + boost::lexical_cast<std::string>(itr2->second) + "," +  boost::lexical_cast<std::string>(target_id);
                                std::vector<long long> relation_type1 = ReturnRelationType(source_id,itr->second);
                                std::vector<long long> relation_type2 = ReturnRelationType(itr->second,itr2->second);
                                std::vector<long long> relation_type3 = ReturnRelationType(itr2->second,target_id);
                                std::vector<std::string> relation_type_string;
                                for(int ii=0;ii<relation_type1.size();++ii){
                                    for(int jj=0;jj<relation_type2.size();++jj){
                                        for(int kk=0;kk<relation_type3.size();++kk){
                                            std::string temp_string = boost::lexical_cast<std::string>(relation_type1[ii]) + "," + boost::lexical_cast<std::string>(relation_type2[jj]) + "," + boost::lexical_cast<std::string>(relation_type3[kk]);
                                            relation_type_string.push_back(temp_string);
                                        }
                                    }
                                }
                                edge_depth_path_relation[edge_id][3][path] = relation_type_string;
                            }
                        }
                    }
                }
            }
            RecoverRelationType(edge_id,source_temp,target_temp,order_temp,tuple_indicator,3,depth_order_vector);
        }// End of depth >= 3

        if(depth >= 4){
            // Create Lagged Version
            std::unordered_map<long long,int> nodes_visited_lagged = nodes_visited;
            // This version uses succint collapsed edgelist
            std::vector<long long> source_temp,target_temp;
            std::vector<int> order_temp;
            auto range = edgelist_collapsed_multimap_train.equal_range(source_id);
            auto range3 = edgelist_collapsed_multimap_train.equal_range(target_id);
            for(auto itr=range.first;itr!=range.second;++itr){// 1 Step
                if(nodes_visited_lagged.find(itr->second)==nodes_visited_lagged.end()){
                    auto range2 = edgelist_collapsed_multimap_train.equal_range(itr->second);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){// 2 Step
                        if(nodes_visited_lagged.find(itr2->second)==nodes_visited_lagged.end()){
                            for(auto itr3=range3.first;itr3!=range3.second;++itr3){// 3 Step
                                if(nodes_visited_lagged.find(itr3->second)==nodes_visited_lagged.end()){
                                    double unique_value1 = ContorPair(double(itr2->second),double(itr3->second));
                                    if(pair_indicator_train.find(unique_value1)!=pair_indicator_train.end()){
                                        // source <-> node1
                                        UpdateVectorBackPath(source_id,itr->second,pairs_included,source_temp,target_temp,order_temp,1,nodes_visited,minimum,1);
                                        // node1 <-> node2
                                        UpdateVectorBackPath(itr->second,itr2->second,pairs_included,source_temp,target_temp,order_temp,2,nodes_visited,minimum,1);
                                        // node2 <-> node3
                                        UpdateVectorBackPath(itr2->second,itr3->second,pairs_included,source_temp,target_temp,order_temp,3,nodes_visited,minimum,1);
                                        // nodes3 <-> target
                                        UpdateVectorBackPath(itr3->second,target_id,pairs_included,source_temp,target_temp,order_temp,4,nodes_visited,minimum,0);

                                        // ADDED
                                        std::string path = boost::lexical_cast<std::string>(source_id)+"," + boost::lexical_cast<std::string>(itr->second) + "," + boost::lexical_cast<std::string>(itr2->second) + "," + boost::lexical_cast<std::string>(itr3->second) + "," + boost::lexical_cast<std::string>(target_id);
                                        std::vector<long long> relation_type1 = ReturnRelationType(source_id,itr->second);
                                        std::vector<long long> relation_type2 = ReturnRelationType(itr->second,itr2->second);
                                        std::vector<long long> relation_type3 = ReturnRelationType(itr2->second,itr3->second);
                                        std::vector<long long> relation_type4 = ReturnRelationType(itr3->second,target_id);
                                        std::vector<std::string> relation_type_string;
                                        for(int ii=0;ii<relation_type1.size();++ii){
                                            for(int jj=0;jj<relation_type2.size();++jj){
                                                for(int kk=0;kk<relation_type3.size();++kk){
                                                    for(int ll=0;ll<relation_type4.size();++ll){
                                                        std::string temp_string = boost::lexical_cast<std::string>(relation_type1[ii]) + "," + boost::lexical_cast<std::string>(relation_type2[jj]) + "," + boost::lexical_cast<std::string>(relation_type3[kk]) + "," + boost::lexical_cast<std::string>(relation_type4[ll]);
                                                        relation_type_string.push_back(temp_string);
                                                    }
                                                }
                                            }
                                        }
                                        edge_depth_path_relation[edge_id][4][path] = relation_type_string;}}}}}}}
            RecoverRelationType(edge_id,source_temp,target_temp,order_temp,tuple_indicator,4,depth_order_vector);
        }// End of depth >= 4
    }// End of BackPath


    void AllBackPath(int depth,int minimum,int steps){// Calculate All BackPath // IN Serialize

        int backpath_calculated = 0;
        int backpath_bypassed = 0;
        for(int i=0;i<source_train.size();++i){
            if(i % 100000 == 0){
                std::cout << "AllBackPath " << i << " Out of " << source_train.size() << " Calculated: " << backpath_calculated << " Bypassed: " << backpath_bypassed << std::endl;}
            std::string source_word = id2node[source_train[i]];
            std::string target_word = id2node[target_train[i]];
            if(core_nodes_dict.find(source_word) != core_nodes_dict.end() && core_nodes_dict.find(target_word) != core_nodes_dict.end()){
                long long source_id = source_train[i];
                long long relation_id = relation_train[i];
                long long target_id = target_train[i];
                BackPath(source_id,relation_id,target_id,depth,1);
                backpath_calculated += 1;
            }else{
                backpath_bypassed += 1;
            }// Pass
        }
        std::cout << "Number of BackPath Calculated: " << backpath_calculated << std::endl;
        double collapsed_inserted = 0;
        // create collapsed_edge_depth_order_backpath
        for(long long i=1;i<collapsed_edge_id_counter;++i){
            if(collapsed_edge_id_is_core.find(i)!=collapsed_edge_id_is_core.end()){
                long long matched_id = collapsed_edge_id2edge_id[i];
                if(edge_depth_order_backpath.find(matched_id)!=edge_depth_order_backpath.end()){
                    for(int j=0;j<depth;++j){
                        for(int k=0;k<j+1;++k){
                            collapsed_edge_depth_order_backpath[i][j+1][k+1] =  edge_depth_order_backpath[matched_id][j+1][k+1];
                            // ADDED
                            collapsed_edge_depth_path_relation[i][j+1] =  edge_depth_path_relation[matched_id][j+1];
                        }
                    }
                    collapsed_edge_depth_order_backpath[i][1][1].push_back(matched_id);
                    // ADDED
                    std::string path = boost::lexical_cast<std::string>(source_train[matched_id]) + "," + boost::lexical_cast<std::string>(target_train[matched_id]);
                    collapsed_edge_depth_path_relation[i][1][path].push_back(boost::lexical_cast<std::string>(relation_train[matched_id]));
                }else{
                    // Only 1 edge
                    collapsed_edge_depth_order_backpath[i][1][1].push_back(matched_id);
                    // ADDED
                    std::string path = boost::lexical_cast<std::string>(source_train[matched_id]) + "," + boost::lexical_cast<std::string>(target_train[matched_id]);

                    collapsed_edge_depth_path_relation[i][1][path].push_back(boost::lexical_cast<std::string>(relation_train[matched_id]));
                }
                collapsed_inserted += 1;
            }
        }
        std::cout << "Collapsed Inserted: " << collapsed_inserted << std::endl;
    }

    void AllBackPath2(int depth,int minimum,int steps){// Calculate All BackPath // IN Serialize

        int backpath_calculated = 0;
        int backpath_bypassed = 0;
        for(int i=0;i<source_train.size();++i){
            if(i % 100000 == 0){
                std::cout << "AllBackPath " << i << " Out of " << source_train.size() << " Calculated: " << backpath_calculated << " Bypassed: " << backpath_bypassed << std::endl;}
            std::string source_word = id2node[source_train[i]];
            std::string target_word = id2node[target_train[i]];
            if(core_nodes_dict.find(source_word) != core_nodes_dict.end() && core_nodes_dict.find(target_word) != core_nodes_dict.end()){
                long long source_id = source_train[i];
                long long relation_id = relation_train[i];
                long long target_id = target_train[i];
                //BackPath(source_id,relation_id,target_id,depth,1);
                backpath_calculated += 1;
            }else{
                backpath_bypassed += 1;
            }// Pass
        }
        std::cout << "Number of BackPath Calculated: " << backpath_calculated << std::endl;

    }

    void AllBackPath3(int depth,int minimum,int steps){// Calculate All BackPath // IN Serialize

        int backpath_calculated = 0;
        int backpath_bypassed = 0;
        double collapsed_inserted = 0;
        std::unordered_map<std::string,int> already_calc;
        std::unordered_map<std::string,int> nodes;
        double num_nodes = 0;

        for(int i=0;i<source_train.size();++i){
            if(i % 100000 == 0){
                std::cout << "AllBackPath " << i << " Out of " << source_train.size() << " Calculated: " << backpath_calculated << " Bypassed: " << backpath_bypassed << std::endl;}
            std::string source_word = id2node[source_train[i]];
            std::string target_word = id2node[target_train[i]];
            std::string pair_word1 = source_word + "," + target_word;
            std::string pair_word2 = target_word + "," + source_word;

            if(core_nodes_dict.find(source_word) != core_nodes_dict.end() &&  core_nodes_dict.find(target_word) != core_nodes_dict.end() && already_calc.find(pair_word1)==already_calc.end() && already_calc.find(pair_word2)==already_calc.end() && source_train[i]!=target_train[i]){

                if(nodes.find(source_word)==nodes.end()){
                    nodes[source_word] = 1;
                    num_nodes += 1.0;
                }
                if(nodes.find(target_word)==nodes.end()){
                    nodes[target_word] = 1;
                    num_nodes += 1.0;
                }

                already_calc[pair_word1] = 1;
                already_calc[pair_word2] = 1;
                long long source_id = source_train[i];
                long long relation_id = relation_train[i];
                long long target_id = target_train[i];

                // This fills edge_depth_order_backpath[matched_id]
                BackPath(source_id,relation_id,target_id,depth,1);

                // Find matched_id
                std::string source_relation_target = boost::lexical_cast<std::string>(source_id) + "," + boost::lexical_cast<std::string>(relation_id) + "," + boost::lexical_cast<std::string>(target_id);
                long long matched_id = edge_dict_train[source_relation_target];

                // Find collapsed_edge_id
                double pair = ContorPair(double(source_id),double(target_id));
                long long collapsed_edge_id = pair2collapsed_edge_id[pair];

                if(edge_depth_order_backpath.find(matched_id)!=edge_depth_order_backpath.end()){
                    for(int j=0;j<depth;++j){
                        for(int k=0;k<j+1;++k){
                            collapsed_edge_depth_order_backpath[collapsed_edge_id][j+1][k+1] =  edge_depth_order_backpath[matched_id][j+1][k+1];
                            // ADDED
                            collapsed_edge_depth_path_relation[collapsed_edge_id][j+1] =  edge_depth_path_relation[matched_id][j+1];
                        }
                    }
                    collapsed_edge_depth_order_backpath[collapsed_edge_id][1][1].push_back(matched_id);
                    // ADDED
                    std::string path = boost::lexical_cast<std::string>(source_train[matched_id]) + "," + boost::lexical_cast<std::string>(target_train[matched_id]);

                    collapsed_edge_depth_path_relation[collapsed_edge_id][1][path].push_back(boost::lexical_cast<std::string>(relation_train[matched_id]));
                }else{
                    // Only 1 edge
                    collapsed_edge_depth_order_backpath[collapsed_edge_id][1][1].push_back(matched_id);
                    // ADDED
                    std::string path = boost::lexical_cast<std::string>(source_train[matched_id]) + "," + boost::lexical_cast<std::string>(target_train[matched_id]);
                    collapsed_edge_depth_path_relation[collapsed_edge_id][1][path].push_back(boost::lexical_cast<std::string>(relation_train[matched_id]));
                }
                /****/
                collapsed_inserted += 1;
                backpath_calculated += 1;
            }else{
                backpath_bypassed += 1;
            }// Pass
        }
        std::cout << "Number of BackPath Calculated: " << backpath_calculated << std::endl;
        std::cout << "Collapsed Inserted: " << collapsed_inserted << std::endl;
        std::cout << "Num Nodes: " << boost::lexical_cast<std::string>(num_nodes) << std::endl;
    }




    // Main function using serialized object //
    // FIN
    void InitializeSparseCoreMatrix(int &steps){

        // Initialize
        id2core_id.clear();
        core_id2id.clear();
        double count_edges_core_matrix = 0;
        core_node_id_counter = 0;
        for(auto itr=core_nodes_dict.begin();itr!=core_nodes_dict.end();++itr){
            if(node2id.find(itr->first)==node2id.end()){
                // We have no relational information forget it
            }else{
                long long source_id = node2id.at(itr->first);
                if(id2core_id.find(source_id)==id2core_id.end()){
                    id2core_id[source_id] = core_node_id_counter;
                    core_id2id[core_node_id_counter] = source_id;
                    core_node_id_counter += 1;}}}

        typedef Eigen::Triplet<float> T;
        std::vector<T> triplet_list;
        for(auto itr=core_nodes_dict.begin();itr!=core_nodes_dict.end();++itr){
            if(node2id.find(itr->first)==node2id.end()){
                // We have no relational information forget it
            }else{
                long long source_id = node2id.at(itr->first);
                if(id2core_id.find(source_id)!=id2core_id.end()){
                    long long core_source_id = id2core_id.at(source_id);
                    auto range = edgelist_collapsed_multimap_train.equal_range(source_id);
                    if(steps==2){
                      //range = edgelist_collapsed_2step_multimap_train.equal_range(source_id);
                    }
                    for(auto itr2=range.first;itr2!=range.second;++itr2){
                        long long target_id = itr2->second;
                        if(id2core_id.find(target_id)!=id2core_id.end()){
                            long long core_target_id = id2core_id.at(target_id);
                            std::string target_word = id2node.at(target_id);
                            if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_source_id != core_target_id){
                                count_edges_core_matrix += 1.0;
                                triplet_list.push_back(T(core_source_id,core_target_id,float(1.0)));
        }}}}}}
        std::cout << "Number of Edges in Sparse Core Matrix: " << count_edges_core_matrix  << std::endl;
        sparse_core_matrix.resize(0,0);
        sparse_core_matrix.setZero();
        sparse_core_matrix.data().squeeze();
        sparse_core_matrix.resize(core_node_id_counter,core_node_id_counter);
        sparse_core_matrix.setFromTriplets(triplet_list.begin(), triplet_list.end());
        for(long long i=0;i<core_node_id_counter;++i){
            //float total = sparse_core_matrix.row(i).sum();
            //sparse_core_matrix.row(i) = sparse_core_matrix.row(i) / total;
            float total = sparse_core_matrix.col(i).sum();
            sparse_core_matrix.col(i) = sparse_core_matrix.col(i) / total;
        }
    }

    void InitializeTopicMatrix(const int dimension,const float alpha){// Initialize edge_matrix

        edge_matrix.setZero(edge_id_counter+1,dimension);// Initialize with zeros
        topic_matrix.setZero(dimension,relation_id_counter);// Initizalize with zeros
        std::random_device rand_dev;
        std::mt19937_64 mt(rand_dev());
        for(long long i=0;i<(edge_id_counter+1);++i){
            if(i<edge_id_counter){
                std::vector<float> x(dimension);
                RandomDirichlet(mt,x,dimension);
                edge_matrix.row(i) = Eigen::Map<Eigen::VectorXf>(&x[0],x.size());
            }else{
                Eigen::VectorXf tempvec;
                tempvec.setZero(dimension);
                edge_matrix.row(i) = tempvec;}}
    }

    void InitializeWeightMatrix(int &max_depth,int &reduce_dimension){
        for(int i=1;i<(max_depth+1);++i){
            for(int j=1;j<(i+1);++j){
                Eigen::MatrixXf temp_matrix = Eigen::MatrixXf::Random(reduce_dimension,edge_matrix.cols());
                depth_order_weight[i][j] = temp_matrix;
                Eigen::MatrixXf zero_matrix;
                zero_matrix.setZero(reduce_dimension,edge_matrix.cols());
                depth_order_derivative_accu[i][j] = zero_matrix;}}
        output_weight = Eigen::MatrixXf::Random(1,reduce_dimension);
        output_derivative_accu.setZero(1,reduce_dimension);
    }


    // DEPRECATED
    void MeanFieldUpdate(const int num_iterations,const float alpha, const float beta,int include_local_same_relation,double threshold,int local_counter_threshold,int relation_counter_threshold){

        // We use relation_edge,edge_parallel,edge_source,edge_target,
        const int dimension = edge_matrix.cols();
        Eigen::VectorXf alphavec = Eigen::VectorXf::Constant(dimension,alpha);
        Eigen::VectorXf betavec = Eigen::VectorXf::Constant(dimension,beta);

        // Random  USE : random_uniform(mt), random_uniform_zero_one(mt)
        std::random_device rand_dev;
        std::mt19937_64 mt(rand_dev());
        std::uniform_int_distribution<> random_uniform(0, edge_id_counter-1);
        std::uniform_real_distribution<double> random_uniform_zero_one(0.0,1.0);
        int temp_counter = 0;

        //double threshold = 1.1;
        for(int i=0;i<num_iterations;++i){// Loop

            // which to update
            long long sample_id = random_uniform(mt);

            // Fraction A
            Eigen::VectorXf A = Eigen::VectorXf::Zero(dimension);
            std::vector<long long> parallel_edge = edge_parallel[sample_id];
            std::vector<long long> source_edge = edge_source[sample_id];
            std::vector<long long> target_edge = edge_target[sample_id];

            A = A + alphavec;

            long long sample_relation;
            if(include_local_same_relation==1){
                sample_relation = -999;
            }else{
                sample_relation = relation_train[sample_id];}

            if(parallel_edge.size()>0 || source_edge.size()>0 || target_edge.size()>0){
                temp_counter = 0;
                for(int j=0;j<parallel_edge.size();++j){
                    double value =  random_uniform_zero_one(mt);
                    if(sample_relation != relation_train[parallel_edge[j]] && value < threshold){
                        Eigen::VectorXf tempvec = edge_matrix.row(parallel_edge[j]);
                        A = A + tempvec;
                        temp_counter += 1;
                    }
                    if(temp_counter >= local_counter_threshold){
                        break;
                    }
                }
                temp_counter = 0;
                for(int j=0;j<source_edge.size();++j){
                    double value = random_uniform_zero_one(mt);
                    if(sample_relation != relation_train[source_edge[j]] && value < threshold){
                        Eigen::VectorXf tempvec = edge_matrix.row(source_edge[j]);
                        A = A + tempvec;
                        temp_counter += 1;
                    }
                    if(temp_counter >= local_counter_threshold){
                        break;
                    }
                }
                temp_counter = 0;
                for(int j=0;j<target_edge.size();++j){
                    double value = random_uniform_zero_one(mt);
                    if(sample_relation != relation_train[target_edge[j]] && value < threshold){
                        Eigen::VectorXf tempvec = edge_matrix.row(target_edge[j]);
                        A = A + tempvec;
                        temp_counter += 1;
                    }
                    if(temp_counter >= local_counter_threshold){
                        break;
                    }
                }
            }
            A = A/A.sum();
            if(A!=A){// Check Point
                std::cout << "Check Point A" << std::endl;}

            // Fraction B // KOKO GA CHIGAUS
            Eigen::VectorXf B = Eigen::VectorXf::Zero(dimension);
            std::vector<long long> edge_relation = relation_edge[relation_train[sample_id]];
            B = B + betavec;
            if(edge_relation.size() > 1){
                temp_counter = 0;
                for(long long j=0;j<edge_relation.size();++j){
                    double value =  random_uniform_zero_one(mt);
                    if(edge_relation[j]!=sample_id && value < threshold){
                        Eigen::VectorXf tempvec = edge_matrix.row(edge_relation[j]);
                        B = B + tempvec;
                        temp_counter += 1;
                    }
                    if(temp_counter >= relation_counter_threshold){
                        break;
                    }
                }

                Eigen::VectorXf aggvec = edge_matrix.colwise().sum() - edge_matrix.row(sample_id);
                aggvec = aggvec + float(relation_id_counter) * betavec;

                // KOKO KOREKA?
                float multiplier = float(edge_relation.size())/float(temp_counter);
                B = multiplier * B;
                B = B.array() / aggvec.array();
                if(B!=B){// Check Point
                    std::cout << "Check Point B" << std::endl;
                    std::cout << "aggvec" << std::endl;
                    std::cout << aggvec << std::endl;
                    std::cout << "B" << std::endl;
                    std::cout << multiplier << std::endl;
                }
            }

            // Normalize
            Eigen::VectorXf C = A.array() * B.array();
            float sum_c = C.sum();
            C = C/sum_c;
            if(C!=C){// Check Point
                std::cout << "Check Point C" << std::endl;
                std::cout << edge_relation.size() << std::endl;
                std::cout << temp_counter << std::endl;
                std::cout << sum_c << std::endl;
            }
            // Replace
            edge_matrix.row(sample_id) = C;
        }
    }

    void MeanFieldUpdateParallel(const int num_iterations,const float alpha, const float beta,int include_local_same_relation,double threshold,int local_counter_threshold,int relation_counter_threshold,int evaluate_every){

        // We use relation_edge,edge_parallel,edge_source,edge_target,
        const int dimension = edge_matrix.cols();
        const Eigen::VectorXf alphavec = Eigen::VectorXf::Constant(dimension,alpha);
        const Eigen::VectorXf betavec = Eigen::VectorXf::Constant(dimension,beta);

        // Random  USE : random_uniform(mt), random_uniform_zero_one(mt)
        std::random_device rand_dev;
        std::mt19937_64 mt(rand_dev());
        std::uniform_int_distribution<> random_uniform(0, edge_id_counter-1);
        std::uniform_real_distribution<double> random_uniform_zero_one(0.0,1.0);
        std::random_device rd;
        std::mt19937 shuffle(rd());

        int keep_running = 1;

        Eigen::VectorXf aggvec = edge_matrix.colwise().sum();
        aggvec = aggvec + float(relation_id_counter) * betavec;

        //double threshold = 1.1;
        #pragma omp parallel // Declare parallel computing
        {
            #pragma omp for // Parallel for
            for(int i=0;i<num_iterations;++i){// Loop
                if(keep_running == 1){

                    int temp_counter = 0;
                    long long sample_id = random_uniform(mt);// which to update

                    // Fraction A
                    Eigen::VectorXf A = Eigen::VectorXf::Zero(dimension);
                    // WE ADDED & TO ENABLE PASS BY REFERENCE
                    std::vector<long long> &parallel_edge = edge_parallel[sample_id];
                    std::vector<long long> &source_edge = edge_source[sample_id];
                    std::vector<long long> &target_edge = edge_target[sample_id];

                    A = A + alphavec;
                    long long sample_relation;
                    if(include_local_same_relation==1){
                        sample_relation = -999;
                    }else{
                        sample_relation = relation_train[sample_id];
                    }
                    if(parallel_edge.size()>0){
                        std::uniform_int_distribution<> random_parallel(0, parallel_edge.size()-1);
                        temp_counter = 0;// set to 0
                        for(int j=0;j<parallel_edge.size();++j){
                            long long place = random_parallel(mt);
                            if(sample_relation != relation_train[parallel_edge[place]]){
                                Eigen::VectorXf tempvec = edge_matrix.row(parallel_edge[place]);
                                A = A + tempvec;
                                temp_counter += 1;
                            }
                            if(temp_counter >= local_counter_threshold){
                                break;
                            }
                        }
                    }
                    if(source_edge.size()>0){
                        std::uniform_int_distribution<> random_source(0, source_edge.size()-1);
                        temp_counter = 0;// set to 0
                        for(int j=0;j<source_edge.size();++j){
                            long long place = random_source(mt);
                            if(sample_relation != relation_train[source_edge[place]]){
                                Eigen::VectorXf tempvec = edge_matrix.row(source_edge[place]);
                                A = A + tempvec;
                                temp_counter += 1;
                            }
                            if(temp_counter >= local_counter_threshold){
                                break;
                            }
                        }
                    }
                    if(target_edge.size()>0){
                        std::uniform_int_distribution<> random_target(0, target_edge.size()-1);
                        temp_counter = 0;// set to 0
                        for(int j=0;j<target_edge.size();++j){
                            long long place = random_target(mt);
                            if(sample_relation != relation_train[target_edge[place]]){
                                Eigen::VectorXf tempvec = edge_matrix.row(target_edge[place]);
                                A = A + tempvec;
                                temp_counter += 1;
                            }
                            if(temp_counter >= local_counter_threshold){
                                break;
                            }
                        }
                    }
                    A = A/A.sum();
                    if(A!=A){// Check Point
                        std::cout << "Check Point A" << std::endl;
                    }

                    // Fraction B // KOKO GA CHIGAUS
                    Eigen::VectorXf B = Eigen::VectorXf::Zero(dimension);
                    B = B + betavec;
                    // WE ADDED & TO ENABLE PASS BY REFERENCE
                    std::vector<long long> &edge_relation = relation_edge[relation_train[sample_id]];
                    if(edge_relation.size() > 1){
                        std::uniform_int_distribution<> random_relation(0,edge_relation.size()-1);
                        temp_counter = 0;
                        for(long long j=0;j<edge_relation.size();++j){
                            long long place = random_relation(mt);
                            if(edge_relation[place]!=sample_id){
                                Eigen::VectorXf tempvec = edge_matrix.row(edge_relation[place]);
                                B = B + tempvec;
                                temp_counter += 1;
                            }
                            if(temp_counter >= relation_counter_threshold){
                                break;
                            }
                        }// LOOP for edge_relation
                        if(temp_counter>0){
                            // KOKO KOREKA?
                            float multiplier = float(edge_relation.size())/float(temp_counter);
                            B = multiplier * B;
                        }
                    }// if(edge_relation.size() > 1)

                    // ORIGINAL VERSION:
                    //Eigen::VectorXf aggvec = edge_matrix.colwise().sum() - edge_matrix.row(sample_id);
                    //aggvec = aggvec + float(relation_id_counter) * betavec;
                    //B = B.array() / aggvec.array();

                    // FASTER VERSION:
                    if(i % evaluate_every == 0){// Update every?
                        Eigen::VectorXf aggvec_private2 = edge_matrix.colwise().sum();
                        aggvec_private2 = aggvec_private2 + float(relation_id_counter) * betavec;
                        #pragma omp critical
                        {
                            aggvec = aggvec_private2;
                        }
                    }
                    Eigen::VectorXf minus_vector = edge_matrix.row(sample_id);
                    Eigen::VectorXf aggvec_private = aggvec - minus_vector;
                    B = B.array() / aggvec_private.array();

                    if(B!=B){// Check Point
                        std::cout << "Check Point B" << std::endl;
                        std::cout << "aggvec" << std::endl;
                        std::cout << aggvec << std::endl;
                    }
                    // Normalize
                    Eigen::VectorXf C = A.array() * B.array();
                    float sum_c = C.sum();
                    C = C/sum_c;
                    if(C!=C){// Check Point
                        std::cout << "Check Point C" << std::endl;
                        std::cout << edge_relation.size() << std::endl;
                        std::cout << temp_counter << std::endl;
                        std::cout << sum_c << std::endl;
                    }
                    #pragma omp critical
                    {
                        edge_matrix.row(sample_id) = C;
                        if(C!=C){// Check Point
                            std::cout << "A" << std::endl;
                            std::cout << A << std::endl;
                            std::cout << "B" << std::endl;
                            std::cout << B << std::endl;
                            keep_running = 0;
                        }
                    }
                }// if keep_running
            }// End of Outer Loop
        }// #pragma omp parallel
    }

    void ExtractTopic(){
        for(long long i=0;i<relation_train.size();++i){// for every edgeid
            Eigen::VectorXf tempvec = edge_matrix.row(i);
            topic_matrix.col(relation_train[i]) = topic_matrix.col(relation_train[i]) + tempvec;}
        Eigen::VectorXf rowsum = topic_matrix.rowwise().sum();
        // I want to avoid this
        for(int i=0;i<topic_matrix.rows();++i){
            topic_matrix.row(i) = topic_matrix.row(i)/rowsum[i];}
    }

    Eigen::MatrixXf EdgeDepthOrderTopic(long long &edge_id,int &depth,int &order){
        std::vector<long long> edge_id_vector = edge_depth_order_backpath[edge_id][depth][order];
        Eigen::MatrixXf feature;
        feature.setZero(edge_matrix.cols(),1);
        for(long long i=0;i<edge_id_vector.size();++i){
            Eigen::VectorXf tempvec = edge_matrix.row(edge_id_vector[i]);
            feature = feature + edge_matrix.row(edge_id_vector[i]).transpose();}
        Eigen::VectorXf colsum = feature.colwise().sum();
        feature = feature/colsum[0];
        return(feature);
    }

    std::vector<long long> EdgeDepthOrderBackPath(long long &source_id,long long &target_id,int &depth,int &order){

        double pair = ContorPair(double(source_id),double(target_id));
        long long collapsed_edge_id = pair2collapsed_edge_id[pair];
        long long edge_id = collapsed_edge_id2edge_id[collapsed_edge_id];

        std::vector<long long> edge_id_vector = edge_depth_order_backpath[edge_id][depth][order];
        return(edge_id_vector);
    }

    // FINAL TARGET
    std::vector<long long> train_indices;
    std::unordered_map<long long,Eigen::SparseMatrix<float, Eigen::ColMajor,long long>> node_id_sparse_core_matrix;
    std::unordered_map<long long,std::unordered_map<long long,std::vector<long long>>> node_id_target_source,node_id_source_target;
    std::unordered_map<long long,std::unordered_map<long long,long long>> node_id_global2local,node_id_local2global;
    std::unordered_map<long long,long long> node_id_num_nodes;

    std::vector<long long> test_indices;
    std::unordered_map<long long,Eigen::SparseMatrix<float, Eigen::ColMajor,long long>> node_id_sparse_core_matrix_test;
    std::unordered_map<long long,std::unordered_map<long long,std::vector<long long>>> node_id_target_source_test,node_id_source_target_test;
    std::unordered_map<long long,std::unordered_map<long long,long long>> node_id_global2local_test,node_id_local2global_test;
    std::unordered_map<long long,long long> node_id_num_nodes_test;

    // FOR PREDICTIO
    Eigen::SparseMatrix<float, Eigen::ColMajor,long long> predict_sparse_core_matrix;
    std::unordered_map<long long,long long> predict_global2local,predict_local2global;
    long long predict_num_nodes;

    Eigen::MatrixXf node_scores;
    std::unordered_map<long long,std::unordered_map<int,std::unordered_map<int,Eigen::MatrixXf>>> node_id_depth_order_derivative;
    std::unordered_map<long long,Eigen::MatrixXf> node_id_output_derivative;

    // FOR Derivative Calculation 1
    std::unordered_map<int,float> derivative_term1;
    std::unordered_map<int,std::unordered_map<int,Eigen::MatrixXf>> depth_order_derivative_all;
    Eigen::MatrixXf output_derivative_all;

    std::vector<long long> use_index;
    std::vector<float> temp_label;
    std::vector<std::string> train_positive_time;

    void ClearTrainPositiveTime(){
        train_positive_time.clear();
        train_positive_time.shrink_to_fit();
    }

    void CreateLocalNetwork(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,int &depth,int &steps){

        // Initialize
        typedef Eigen::Triplet<float> T;

        // Initialize
        train_indices.clear();train_indices.shrink_to_fit();
        node_id_sparse_core_matrix.clear();
        node_id_target_source.clear();
        node_id_source_target.clear();
        node_id_global2local.clear();
        node_id_local2global.clear();
        node_id_num_nodes.clear();

        test_indices.clear();test_indices.shrink_to_fit();
        node_id_sparse_core_matrix_test.clear();
        node_id_target_source_test.clear();
        node_id_source_target_test.clear();
        node_id_global2local_test.clear();
        node_id_local2global_test.clear();
        node_id_num_nodes_test.clear();

        predict_global2local.clear();
        predict_local2global.clear();
        predict_num_nodes = 0;

        int hit_counter = 0;

        // include only
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                if(train_positive_time[i] < train_deve_split){
                    train_positive_dict[train_positive(i)] = train_positive_time[i];
                }else{
                    devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}

        for(auto itr=train_positive_dict.begin();itr!=train_positive_dict.end();++itr){

            std::unordered_map<int,std::unordered_map<int,int>> depth_already_visited;
            std::unordered_map<long long,long long> global2local,local2global;
            long long start_id = itr->first;// global id
            std::string start_time = itr->second;

            depth_already_visited[0][start_id] = 1;
            global2local[start_id] = 0;
            local2global[0] = start_id;
            long long local_node_counter = 1;
            int hit = 0;
            int hantei0,hantei1,hantei2,hantei3;
            std::vector<T> triplet_list;
            Eigen::SparseMatrix<float, Eigen::ColMajor,long long> sparse_core_matrix_temp;
            std::unordered_map<long long,std::vector<long long>> target_source,source_target;
            std::vector<long long> temp_target_id;

            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    hantei0 = 0;
                    if(depth_already_visited[0].find(target_id) == depth_already_visited[0].end()){
                        hantei0 = 1;
                    }
                    if(hantei0==1){
                        if(global2local.find(target_id)==global2local.end()){
                            global2local[target_id] = local_node_counter;
                            local2global[local_node_counter] = target_id;
                            local_node_counter = local_node_counter + 1;
                            depth_already_visited[1][target_id] = 1;
                        }
                        triplet_list.push_back(T(global2local[target_id],global2local[start_id],float(1.0)));

                        if(target_source.find(global2local[target_id])==target_source.end()){
                            std::vector<long long> temp_vector;
                            temp_vector.push_back(global2local[start_id]);
                            target_source[global2local[target_id]] = temp_vector;
                        }else{
                            target_source[global2local[target_id]].push_back(global2local[start_id]);
                        }
                        if(source_target.find(global2local[start_id])==source_target.end()){
                            std::vector<long long> temp_vector;
                            temp_vector.push_back(global2local[target_id]);
                            source_target[global2local[start_id]] = temp_vector;
                        }else{
                            source_target[global2local[start_id]].push_back(global2local[target_id]);
                        }

                        // if target id is in devel
                        if(devel_positive_dict.find(target_id)!=devel_positive_dict.end()){
                            hit = 1;
                        }
                        temp_target_id.push_back(target_id);
                    }// if(hantei0==1){
                }// if(hantei_core==1){
            }// for(auto itr2=range.first

            if(local_node_counter>1){
            //if(hit==1 && local_node_counter>1){

                // start_id GLOBAL ID
                node_id_num_nodes[start_id] = local_node_counter;
                sparse_core_matrix_temp.resize(local_node_counter,local_node_counter);
                sparse_core_matrix_temp.setFromTriplets(triplet_list.begin(), triplet_list.end());
                node_id_sparse_core_matrix[start_id] = sparse_core_matrix_temp;
                node_id_global2local[start_id] = global2local;
                node_id_local2global[start_id] = local2global;
                node_id_target_source[start_id] = target_source;
                node_id_source_target[start_id] = source_target;
                train_indices.push_back(start_id);
                hit_counter += 1;
            }// if(hit==1 && local_node_counter>1)
        }// LOOP positiveindices



        //// FOR TEST SET ////
        std::unordered_map<long long,std::string> test_positive_dict;
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                test_positive_dict[test_positive(i)] = "1";
            }
        }

        std::vector<T> predict_triplet_list;
        long long predict_node_counter = 0;
        for(int i=0;i<train_positive.size();++i){// INCLUDES train and devel
            long long start_id = train_positive(i);
            predict_global2local[start_id] = predict_node_counter;
            predict_local2global[predict_node_counter] = start_id;
            predict_node_counter += 1;
        }

        for(int i=0;i<train_positive.size();++i){

            std::unordered_map<int,std::unordered_map<int,int>> depth_already_visited;
            std::unordered_map<long long,long long> global2local,local2global;
            long long start_id = train_positive(i);
            //std::string start_time = itr->second;

            depth_already_visited[0][start_id] = 1;
            global2local[start_id] = 0;
            local2global[0] = start_id;
            long long local_node_counter = 1;
            int hit,hantei0,hantei1,hantei2,hantei3;
            std::vector<T> triplet_list;
            Eigen::SparseMatrix<float, Eigen::ColMajor,long long> sparse_core_matrix_temp;
            std::unordered_map<long long,std::vector<long long>> target_source,source_target;
            std::vector<long long> temp_target_id;

            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    hantei0 = 0;
                    if(depth_already_visited[0].find(target_id) == depth_already_visited[0].end()){
                        hantei0 = 1;
                    }
                    if(hantei0==1){
                        if(global2local.find(target_id)==global2local.end()){
                            global2local[target_id] = local_node_counter;
                            local2global[local_node_counter] = target_id;
                            local_node_counter = local_node_counter + 1;
                            depth_already_visited[1][target_id] = 1;
                        }
                        triplet_list.push_back(T(global2local[target_id],global2local[start_id],float(1.0)));

                        if(target_source.find(global2local[target_id])==target_source.end()){
                            std::vector<long long> temp_vector;
                            temp_vector.push_back(global2local[start_id]);
                            target_source[global2local[target_id]] = temp_vector;
                        }else{
                            target_source[global2local[target_id]].push_back(global2local[start_id]);
                        }
                        if(source_target.find(global2local[start_id])==source_target.end()){
                            std::vector<long long> temp_vector;
                            temp_vector.push_back(global2local[target_id]);
                            source_target[global2local[start_id]] = temp_vector;
                        }else{
                            source_target[global2local[start_id]].push_back(global2local[target_id]);
                        }
                        temp_target_id.push_back(target_id);
                    }// if(hantei0==1){
                }// if(hantei_core==1){
            }// for(auto itr2=range.first

            if( local_node_counter>9 ){

                // start_id GLOBAL ID
                node_id_num_nodes_test[start_id] = local_node_counter;
                sparse_core_matrix_temp.resize(local_node_counter,local_node_counter);
                sparse_core_matrix_temp.setFromTriplets(triplet_list.begin(),triplet_list.end());
                node_id_sparse_core_matrix_test[start_id] = sparse_core_matrix_temp;
                node_id_global2local_test[start_id] = global2local;
                node_id_local2global_test[start_id] = local2global;
                node_id_target_source_test[start_id] = target_source;
                node_id_source_target_test[start_id] = source_target;
                test_indices.push_back(start_id);
                hit_counter += 1;

                for(int j=0;j<temp_target_id.size();++j){
                    long long target_id = temp_target_id[j];
                    if(predict_global2local.find(target_id)==predict_global2local.end()){
                        predict_global2local[target_id] = predict_node_counter;
                        predict_local2global[predict_node_counter] = target_id;
                        predict_node_counter = predict_node_counter + 1;
                    }
                    predict_triplet_list.push_back(T(predict_global2local[target_id],predict_global2local[start_id],float(1.0)));
                }
            }// if(hit==1 && local_node_counter>1)
        }// LOOP positiveindices

        predict_sparse_core_matrix.resize(predict_node_counter,predict_node_counter);
        predict_sparse_core_matrix.setFromTriplets(predict_triplet_list.begin(),predict_triplet_list.end());
        predict_num_nodes = predict_node_counter;
        std::cout << "Number Hit " << hit_counter << std::endl;
    }

    // DEPRECATED
    Eigen::MatrixXf feature_matrix,target_matrix,feature_matrix_test,target_matrix_test,target_indices_test;

    //// Label Propagation Objects ////
    std::unordered_map<long long,long long> label_prop_id2id,id2label_prop_id;
    std::vector<long long> label_prop_row,label_prop_col;
    Eigen::MatrixXi label_prop_row_col;
    Eigen::MatrixXf label_prop_weight;
    long long label_prop_counter;
    Eigen::MatrixXf label_prop_inverse_train,label_prop_inverse_test;
    Eigen::MatrixXf label_prop_I_train,label_prop_I_test;// NEW!
    Eigen::MatrixXf y_init_train,y_init_test,y_full;
    std::vector<long long> eval_indices_train,eval_indices_test;
    long long train_number,devel_number,test_number;

    std::string CompareTimeString(std::string time_a,std::string time_b){
        std::string output_string;
        if(time_a < time_b){
            output_string = time_b;
        }else{
            output_string = time_a;
        }
        return(output_string);
    }

    Eigen::MatrixXf edge_matrix_one_hot;


    // USED
    void CreateSparseWeightOneHotBackPath2(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon,float &cut_threshold,int &zero_one){
        node2degree.clear();

        // Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;
                        }
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;
                    }
                }
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                    temp_counter += 1;}}
            //// KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i){
            y_init_train(i,0) = 1.0;
        }
        for(long long i=train_number;i<label_prop_counter;++i){
            eval_indices_train.push_back(i);
        }
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i){
            y_init_test(i,0) = 1.0;
        }
        for(long long i=devel_number;i<label_prop_counter;++i){
            eval_indices_test.push_back(i);
        }
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i){
            y_full(i,0) = 1;
        }
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;

        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        label_prop_I_train.resize(0,0);
        label_prop_I_train.setZero();
        label_prop_I_train.setZero(label_prop_counter,1);
        label_prop_I_test.resize(0,0);
        label_prop_I_test.setZero();
        label_prop_I_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1 && start_id != target_id){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;
                }
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
            }

            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_I_train(i,0) = (1.0 +  mu * epsilon);
            }else{
                label_prop_I_train(i,0) = ( mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_I_test(i,0) = (1.0 +  mu * epsilon);
            }else{
                label_prop_I_test(i,0) = (mu * epsilon);
            }
        }

        //label_prop_weight.setZero(num_rows,num_matrix*edge_matrix.cols());
        label_prop_weight.resize(0,0);
        label_prop_weight.setZero();
        //label_prop_weight.setZero(num_rows,num_matrix*relation_id_counter);
        label_prop_weight.setZero(num_rows,6*relation_id_counter);

        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1 && start_id != target_id){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    std::vector<float> tempvec;
                    for(int k=1;k<(max_depth+1);++k){
                        for(int l=1;l<(k+1);++l){
                            std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                            Eigen::MatrixXf middle1;
                            middle1.setZero(relation_id_counter,1);
                            // NEW
                            int include_or_cut = 1;
                            std::unordered_map<long long,int> temp_map;
                            temp_map[start_id] = 1;
                            temp_map[target_id] = 1;
                            std::vector<long long> consider_list;
                            //for(int m=0;m<indices.size();++m){
                            //    if(temp_map.find(source_train[indices[m]])==temp_map.end()){
                            //        consider_list.push_back(source_train[indices[m]]);
                            //    }
                            //    if(temp_map.find(target_train[indices[m]])==temp_map.end()){
                            //        consider_list.push_back(target_train[indices[m]]);
                            //    }
                            //}
                            //for(int m=0;m<consider_list.size();++m){
                            //    float count = 0.0;
                            //    if(node2degree.find(consider_list[m])==node2degree.end()){
                            //        auto range4 = edgelist_collapsed_multimap_train.equal_range(consider_list[m]);
                            //        for(auto itr4=range4.first;itr4!=range4.second;++itr4){
                            //            count += 1.0;
                            //        }
                            //        node2degree[consider_list[m]] = count;
                            //    }else{
                            //        count = node2degree[consider_list[m]];
                            //    }
                            //    if(log(count+1.0) > cut_threshold){
                            //        include_or_cut = 0;
                            //    }
                            //}
                            if(include_or_cut==1){
                                for(int m=0;m<indices.size();++m){
                                    //Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[m]);
                                    Eigen::MatrixXf temp_edge_matrix;
                                    temp_edge_matrix.setZero(relation_id_counter,1);
                                    int place = int(relation_train[indices[m]]);
                                    temp_edge_matrix(place,0) = 1.0;
                                    middle1 = middle1 + temp_edge_matrix;
                                }
                                if(indices.size()>0){
                                    //middle1 = middle1 / middle1.sum();
                                }
                            }
                            for(int m=0;m<relation_id_counter;++m){
                                tempvec.push_back(middle1(m,0));
                            }
                        }
                    }// end of depth order loop

                    // Old version
                    //Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());

                    // NEW! : Symmetry
                    std::vector<float> tempvec2;
                    for(int n=0;n<6*int(relation_id_counter);++n) tempvec2.push_back(0.0);
                    for(int n=0;n<tempvec.size();++n){
                        double hantei = double(n)/double(relation_id_counter);
                        int m = n % int(relation_id_counter);// POSSIBLE ERROR
                        if(hantei < 1.0){
                            tempvec2[n] = tempvec[n];
                        }else if( (1.0 <= hantei) && (hantei < 3.0) ){// Depth 2
                            tempvec2[int(relation_id_counter) + m] += tempvec[n];
                        }else if( ( (3.0 <= hantei) && (hantei < 4.0) ) || ( (5.0 <= hantei) && (hantei < 6.0)) ){// Depth 3
                            tempvec2[2*int(relation_id_counter) + m] += tempvec[n];
                        }else if( ((4.0 <= hantei) && (hantei < 5.0)) ){// Depth 3
                            tempvec2[3*int(relation_id_counter) + m] += tempvec[n];
                        }else if( ((6.0 <= hantei) && (hantei < 7.0)) || (9.0 <= hantei) ){
                            tempvec2[4*int(relation_id_counter) + m] += tempvec[n];
                        }else{
                            tempvec2[5*int(relation_id_counter) + m] += tempvec[n];
                        }
                    }

                    if(zero_one==1){// 0 - 1 : Quantile にするならこれは飛ばす
                        for(int m = 0;m < tempvec2.size();++m){
                            if(tempvec2[m] > 0){
                                tempvec2[m] = 1.0;}}
                    }else{// take logarithm
                        for(int m = 0;m < tempvec2.size();++m){
                            if(tempvec2[m] > 0){
                                double value_d = log(double(tempvec2[m]));
                                tempvec2[m] = float(value_d);}}
                    }
                    Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec2[0],tempvec2.size());

                    // ATOHA ONAJI
                    label_prop_weight.row(row_counter) = feature_vector;
                    row_counter += 1;
                }
            }
        }


    }

    std::unordered_map<double,int> local_pair2index;

    void CalculateCommon(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon,float &cut_threshold,int &zero_one){
        local_pair2index.clear();
        node2degree.clear();
        // Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;
                        }
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;
                    }
                }
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                    temp_counter += 1;}}
            //// KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i){
            y_init_train(i,0) = 1.0;
        }
        for(long long i=train_number;i<label_prop_counter;++i){
            eval_indices_train.push_back(i);
        }
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i){
            y_init_test(i,0) = 1.0;
        }
        for(long long i=devel_number;i<label_prop_counter;++i){
            eval_indices_test.push_back(i);
        }
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i){
            y_full(i,0) = 1;
        }
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;

        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1 && start_id != target_id){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);
                    //double pair = ContorPair(double(i),double(local_target));
                    //local_pair2index[pair] = label_prop_row.size() - 1;
                }}}
    }

    void CreateLocalPair2Index(Eigen::MatrixXi &label_prop_row_col0){
        local_pair2index.clear();
        for(int i=0;i<label_prop_row_col0.rows();++i){
            double pair1 = ContorPair(double(label_prop_row_col0(i,0)),
            double(label_prop_row_col0(i,1)));
            local_pair2index[pair1] = i;
        }
    }

    std::vector<long long> nearby_source,nearby_target;
    std::vector<float> nearby_weight;

    void FindNearbyLabel(const int &local_start_id,Eigen::VectorXf &edge_weight_data,
        Eigen::VectorXi &train_positive,Eigen::VectorXi &test_positive,const int &depth){

        // Initialize
        nearby_source.clear();nearby_source.shrink_to_fit();
        nearby_target.clear();nearby_target.shrink_to_fit();
        nearby_weight.clear();nearby_weight.shrink_to_fit();

        std::unordered_map<long long,int> train_positive_dict;
        std::unordered_map<long long,int> test_positive_dict;
        for(int i=0;i<train_positive.size();++i) train_positive_dict[train_positive(i)] = 1;
        for(int i=0;i<test_positive.size();++i) test_positive_dict[test_positive(i)] = 1;

        // with_label,0,1,2
        std::unordered_map<long long,int> nodes_visited,nodes_visited0,nodes_visited1,nodes_visited2;
        std::unordered_map<long long,int> nodes_value;

        // depth0
        long long start_id = label_prop_id2id[local_start_id];
        nodes_visited0[start_id] = 1;
        auto range = edgelist_collapsed_multimap_train.equal_range(start_id);

        if(depth>0){// depth1
            for(auto itr=range.first;itr!=range.second;++itr){
                long long target_id = itr->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1 && start_id != target_id){
                    //nodes_visited1[target_id] = 1;// depth 1
                    long long local_target_id = id2label_prop_id[target_id];
                    double pair1 = ContorPair(double(local_start_id),double(local_target_id));
                    int index1 = -1;
                    if(local_pair2index.find(pair1)!=local_pair2index.end()){
                        index1 = local_pair2index[pair1];
                    }else{
                        std::cout << "Naiyo!" << std::endl;
                    }
                    if(1==1){
                    //if(train_positive_dict.find(target_id)!=train_positive_dict.end()){
                        //nodes_visited[target_id] = 1;
                        nearby_source.push_back(local_start_id);
                        nearby_target.push_back(local_target_id);
                        nearby_weight.push_back(edge_weight_data(index1));}}}}

        if(depth>1){// depth 2
            for(auto itr=range.first;itr!=range.second;++itr){
                long long target_id = itr->second;
                //if(nodes_visited.find(target_id)==nodes_visited.end()){
                if(1==1){
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        hantei_core = 1;}
                    if(hantei_core==1 && start_id != target_id){
                        long long local_target_id = id2label_prop_id[target_id];
                        double pair1 = ContorPair(double(local_start_id),double(local_target_id));
                        int index1 = -1;
                        if(local_pair2index.find(pair1)!=local_pair2index.end()){
                            index1 = local_pair2index[pair1];
                        }else{
                            std::cout << "Naiyo!" << std::endl;
                        }
                        auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
                        for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                            long long target_id2 = itr2->second;
                            if( nodes_visited0.find(target_id2)==nodes_visited0.end() && nodes_visited1.find(target_id2)==nodes_visited1.end() ){
                                hantei_core = 0;
                                std::string target_word2 = id2node[target_id2];
                                if(core_nodes_dict.find(target_word2)!=core_nodes_dict.end()){
                                    hantei_core = 1;}
                                if(hantei_core==1 && target_id != target_id2){
                                    //nodes_visited2[target_id2] = 1;
                                    long long local_target_id2 = id2label_prop_id[target_id2];
                                    int index2 = -1;
                                    double pair2 = ContorPair(double(local_target_id),double(local_target_id2));
                                    if(local_pair2index.find(pair2)!=local_pair2index.end()){
                                        index2 = local_pair2index[pair2];
                                    }else{
                                        std::cout << "Naiyo!" << std::endl;
                                    }
                                    if(1==1){
                                    //if(train_positive_dict.find(target_id2)!=train_positive_dict.end()){
                                        //nodes_visited[target_id2] = 1;
                                        nearby_source.push_back(local_start_id);
                                        nearby_target.push_back(local_target_id);
                                        nearby_weight.push_back(edge_weight_data(index1));
                                        nearby_source.push_back(local_target_id);
                                        nearby_target.push_back(local_target_id2);
                                        nearby_weight.push_back(edge_weight_data(index2));}}}}}}}}

        if(depth > 2){// depth 3
            for(auto itr=range.first;itr!=range.second;++itr){
                long long target_id = itr->second;
                if(1==1){
                //if(nodes_visited.find(target_id)==nodes_visited.end()){
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        hantei_core = 1;}
                    if(hantei_core==1 && start_id != target_id){
                        long long local_target_id = id2label_prop_id[target_id];
                        double pair1 = ContorPair(double(local_start_id),double(local_target_id));
                        int index1 = -1;
                        if(local_pair2index.find(pair1)!=local_pair2index.end()){
                            index1 = local_pair2index[pair1];
                        }else{
                            std::cout << "Naiyo!" << std::endl;
                        }
                        auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
                        for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                            long long target_id2 = itr2->second;
                            if( nodes_visited.find(target_id2)==nodes_visited.end() && nodes_visited0.find(target_id2)==nodes_visited0.end() && nodes_visited1.find(target_id2)==nodes_visited1.end() ){
                                hantei_core = 0;
                                std::string target_word2 = id2node[target_id2];
                                if(core_nodes_dict.find(target_word2)!=core_nodes_dict.end()){
                                    hantei_core = 1;}
                                if(hantei_core==1 && target_id != target_id2){
                                    long long local_target_id2 = id2label_prop_id[target_id2];
                                    int index2 = -1;
                                    double pair2 = ContorPair(double(local_target_id),double(local_target_id2));
                                    if(local_pair2index.find(pair2)!=local_pair2index.end()){
                                        index2 = local_pair2index[pair2];
                                    }else{
                                        std::cout << "Naiyo!" << std::endl;
                                    }
                                    auto range3 = edgelist_collapsed_multimap_train.equal_range(target_id2);
                                    for(auto itr3=range3.first;itr3!=range3.second;++itr3){
                                        long long target_id3 = itr3->second;
                                        if(nodes_visited.find(target_id3)==nodes_visited.end() &&
                                          nodes_visited0.find(target_id3)==nodes_visited0.end() && nodes_visited1.find(target_id3)==nodes_visited1.end() && nodes_visited2.find(target_id3)==nodes_visited2.end() ){
                                            hantei_core = 0;
                                            std::string target_word3 = id2node[target_id3];
                                            if(core_nodes_dict.find(target_word3)!=core_nodes_dict.end()){
                                                hantei_core = 1;}
                                            if(hantei_core==1 && target_id2 != target_id3){
                                                long long local_target_id3 = id2label_prop_id[target_id3];
                                                int index3 = -1;
                                                double pair3 = ContorPair(double(local_target_id2),double(local_target_id3));
                                                if(local_pair2index.find(pair3)!=local_pair2index.end()){
                                                    index3 = local_pair2index[pair3];
                                                }else{
                                                    std::cout << "Naiyo!" << std::endl;
                                                }
                                                //if(train_positive_dict.find(target_id3)!=train_positive_dict.end()){
                                                if(1==1){
                                                    //nodes_visited[target_id3] = 1;
                                                    nearby_source.push_back(local_start_id);
                                                    nearby_target.push_back(local_target_id);
                                                    nearby_weight.push_back(edge_weight_data(index1));
                                                    nearby_source.push_back(local_target_id);
                                                    nearby_target.push_back(local_target_id2);
                                                    nearby_weight.push_back(edge_weight_data(index2));
                                                    nearby_source.push_back(local_target_id2);
                                                    nearby_target.push_back(local_target_id3);
                                                    nearby_weight.push_back(edge_weight_data(index3));}}}}}}}}}}}

    }

    std::vector<float> lp_all_pre,lp_all_test;

    Eigen::SparseMatrix<float, Eigen::ColMajor,long long> sparse_all_matrix,sparse_all_matrix_temp;

    void LPAll(int inner_iteration,Eigen::VectorXi &train_positive,Eigen::VectorXi &test_positive,float &num_edge_threshold,float &mu,float &epsilon){

      std::unordered_map<long long,std::string> train_positive_dict,test_positive_dict;
      std::unordered_map<long long,std::string> threshold_dict;
      label_prop_id2id.clear();
      id2label_prop_id.clear();
      label_prop_row.clear();label_prop_row.shrink_to_fit();
      label_prop_col.clear();label_prop_col.shrink_to_fit();
      label_prop_counter=0;

      // Train
      for(int i=0;i<train_positive.size();++i){
          train_positive_dict[train_positive(i)] = "1";
          if(id2core_id.find(train_positive(i))!=id2core_id.end()){
              long long start_id = train_positive(i);
              float temp_counter = 0;
              auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
              for(auto itr2=range.first;itr2!=range.second;++itr2){
                  long long target_id = itr2->second;
                  std::string source_word = id2node[start_id];
                  std::string target_word = id2node[target_id];
                  if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                      temp_counter += 1;}}
              // KOKO
              if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                  threshold_dict[train_positive(i)] = "1";
                  if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                      id2label_prop_id[train_positive(i)] = label_prop_counter;
                      label_prop_id2id[label_prop_counter] = train_positive(i);
                      label_prop_counter += 1;}}}}
      train_number = label_prop_counter;
      std::cout << "Num Train:" << boost::lexical_cast<std::string>(train_number) << std::endl;

      // Test
      for(int i=0;i<test_positive.size();++i){
          test_positive_dict[test_positive(i)] = "1";
          if(id2core_id.find(test_positive(i))!=id2core_id.end()){
              long long start_id = test_positive(i);
              float temp_counter = 0;
              auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
              for(auto itr2=range.first;itr2!=range.second;++itr2){
                  long long target_id = itr2->second;
                  std::string source_word = id2node[start_id];
                  std::string target_word = id2node[target_id];
                  if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                      temp_counter += 1;}}
              // KOKO
              if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                  threshold_dict[train_positive(i)] = "1";
                  if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                      id2label_prop_id[test_positive(i)] = label_prop_counter;
                      label_prop_id2id[label_prop_counter] = test_positive(i);
                      label_prop_counter += 1;}}}}
      test_number = label_prop_counter;
      std::cout << "Num Train:" << boost::lexical_cast<std::string>(test_number - train_number) << std::endl;

      // Rest
      for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
          long long start_id = itr->first;
          auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
          float temp_counter = 0;
          for(auto itr2=range.first;itr2!=range.second;++itr2){
              long long target_id = itr2->second;
              std::string source_word = id2node[start_id];
              std::string target_word = id2node[target_id];
              if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                  temp_counter += 1;}}
          //// KOKO
          if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
              threshold_dict[itr->first] = "1";}}

       /*
              if(train_positive_dict.find(itr->first)==train_positive_dict.end() &&  test_positive_dict.find(itr->first)==test_positive_dict.end()){
                  if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                      id2label_prop_id[itr->first] = label_prop_counter;
                      label_prop_id2id[label_prop_counter] = itr->first;
                      label_prop_counter += 1;}}}}
        /****/

        typedef Eigen::Triplet<float> T;
        std::vector<T> triplet_list;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(node_id_counter,1);
        for(long i=0;i<node_id_counter;++i){
            if(i % 10000 == 0){
                std::cout << "Finished: " << boost::lexical_cast<std::string>(i) << " Out of: " << boost::lexical_cast<std::string>(source_train.size()) << std::endl;
            }
            long long start_id = i;
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                temp_counter += 1;
            }
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                triplet_list.push_back(T(target_id,start_id,float(1)));
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu*epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu*epsilon);
            }
        }

        sparse_all_matrix.resize(0,0);
        sparse_all_matrix.setZero();
        sparse_all_matrix.data().squeeze();
        sparse_all_matrix.resize(node_id_counter,node_id_counter);
        sparse_all_matrix.setFromTriplets(triplet_list.begin(),triplet_list.end());
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(node_id_counter,1);
        int hit_counter = 0;
        for(long long i=0;i<node_id_counter;++i){
            if(i % 100000 == 0){
                std::cout << "TrainPositive: " << boost::lexical_cast<std::string>(i) << " Out of: " << boost::lexical_cast<std::string>(node_id_counter) << std::endl;
            }
            if(train_positive_dict.find(i)!=train_positive_dict.end()){
                y_init_train(i,0) = 1.0;
                hit_counter += 1;
            }
        }
        std::cout << "Num Hit: " << hit_counter << std::endl;

        Eigen::MatrixXf f;
        f.resize(0,0);
        f.setZero();
        f.setZero(node_id_counter,1);

        // Random  USE : random_uniform(mt), random_uniform_zero_one(mt)
        std::random_device rand_dev;
        std::mt19937_64 mt(rand_dev());
        std::uniform_real_distribution<float> random_uniform_left_right(-1.0,1.0);
        // which to update
        for(int i=0;i<node_id_counter;++i){
            f(i,0) =random_uniform_left_right(mt);
        }

        //f = np.random.uniform(bound_left,bound_right,len(y_init_train))
        //f = f.astype("float32")
        //f = tf.reshape(f,[len(y_init_train),-1])
        //for i in range(inner_iteration):
        //    f0 = f
        //    tempB = y_init_train + mu * tf.sparse_tensor_dense_matmul(label_prop_matrix,f0)
        //    tempB = tf.reshape(tempB,(len(y_full),-1))
        //    f = tf.sparse_tensor_dense_matmul(
        //              A_inv,tempB,adjoint_a=False,adjoint_b=False,name=None)
        for(int i=0;i<inner_iteration;++i){
            std::cout << "Iteration: " << boost::lexical_cast<std::string>(i+1) << std::endl;
            Eigen::MatrixXf tempB = y_init_train + mu * sparse_all_matrix * f;
            for(int j=0;j<node_id_counter;++j){
                f(i,0) = label_prop_inverse_train(i,0) * tempB(i,0);
            }
        }
        lp_all_pre.clear();lp_all_pre.shrink_to_fit();
        lp_all_test.clear();lp_all_test.shrink_to_fit();
        for(int i=0;i<node_id_counter;++i){
            if(threshold_dict.find(i)!=threshold_dict.end() && train_positive_dict.find(i)==train_positive_dict.end()){
                lp_all_pre.push_back(f(i,0));
                if(test_positive_dict.find(i)!=test_positive_dict.end()){
                    lp_all_test.push_back(1.0);
                }else{
                    lp_all_test.push_back(0.0);
                }
            }
        }
    }

    Eigen::MatrixXf final_feature;
    std::vector<long long> final_source,final_target;

    void TrackLabel(Eigen::VectorXi &train_indices,Eigen::VectorXi &test_indices,int &max_depth){

        final_source.clear();final_source.shrink_to_fit();
        final_target.clear();final_target.shrink_to_fit();

        std::unordered_map<long long,std::string> train_positive_dict,test_positive_dict;

        for(int i=0;i<train_indices.size();++i){
            train_positive_dict[train_indices(i)] = "1";
        }
        for(int i=0;i<test_indices.size();++i){
            test_positive_dict[test_indices(i)] = "1";
        }

        // Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}

        int row_counter = 0;
        for(int i=0;i<test_indices.size();++i){
            long long start_id = test_indices(i);
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){

                    if(train_positive_dict.find(target_id)!=train_positive_dict.end()){
                        row_counter += 1;
                    }

                    auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
                    for(auto itr3=range2.first;itr3!=range2.second;++itr3){
                        long long target_id2 = itr3->second;
                        std::string target_word2 = id2node[target_id2];
                        if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(target_word2)!=core_nodes_dict.end()){
                            if(train_positive_dict.find(target_id2)!=train_positive_dict.end()){
                                row_counter += 1;
                              }
                        }
                    }
                }
            }
        }
        final_feature.resize(0,0);
        final_feature.setZero();
        final_feature.setZero(row_counter,num_matrix*relation_id_counter);
        row_counter = 0;
        for(int i=0;i<test_indices.size();++i){
            long long start_id = test_indices(i);
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){

                    if(train_positive_dict.find(target_id)!=train_positive_dict.end()){

                        final_source.push_back(start_id);
                        final_target.push_back(target_id);
                        double pair = ContorPair(double(start_id),double(target_id));
                        long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                        std::vector<float> tempvec;
                        for(int k=1;k<(max_depth+1);++k){
                            for(int l=1;l<(k+1);++l){
                                std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                                Eigen::MatrixXf middle1;
                                middle1.setZero(relation_id_counter,1);
                                for(int m=0;m<indices.size();++m){
                                    //Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[m]);
                                    Eigen::MatrixXf temp_edge_matrix;
                                    temp_edge_matrix.setZero(relation_id_counter,1);
                                    int place = int(relation_train[indices[m]]);
                                    temp_edge_matrix(place,0) = 1.0;
                                    middle1 = middle1 + temp_edge_matrix;
                                }
                                if(indices.size()>0){
                                    middle1 = middle1 / middle1.sum();
                                }
                                for(int m=0;m<relation_id_counter;++m){
                                    tempvec.push_back(middle1(m,0));
                                }
                            }
                        }// end of depth order loop
                        Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());
                        final_feature.row(row_counter) = feature_vector;
                        row_counter += 1;
                    }

                    auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
                    for(auto itr3=range2.first;itr3!=range2.second;++itr3){
                        long long target_id2 = itr3->second;
                        std::string target_word2 = id2node[target_id2];
                        if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(target_word2)!=core_nodes_dict.end()){
                            if(train_positive_dict.find(target_id2)!=train_positive_dict.end()){
                                final_source.push_back(target_id);
                                final_target.push_back(target_id2);
                                double pair = ContorPair(double(start_id),double(target_id));
                                long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                                std::vector<float> tempvec;
                                for(int k=1;k<(max_depth+1);++k){
                                    for(int l=1;l<(k+1);++l){
                                        std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                                        Eigen::MatrixXf middle1;
                                        middle1.setZero(relation_id_counter,1);
                                        for(int m=0;m<indices.size();++m){
                                            //Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[m]);
                                            Eigen::MatrixXf temp_edge_matrix;
                                            temp_edge_matrix.setZero(relation_id_counter,1);
                                            int place = int(relation_train[indices[m]]);
                                            temp_edge_matrix(place,0) = 1.0;
                                            middle1 = middle1 + temp_edge_matrix;
                                        }
                                        if(indices.size()>0){
                                            middle1 = middle1 / middle1.sum();
                                        }
                                        for(int m=0;m<relation_id_counter;++m){
                                            tempvec.push_back(middle1(m,0));
                                        }
                                    }
                                }// end of depth order loop
                                Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());
                                final_feature.row(row_counter) = feature_vector;
                                row_counter += 1;

                            }
                        }
                    }
                }
            }
        }



        for(int i=0;i<test_indices.size();++i){
            long long start_id = test_indices(i);
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){

                }
            }
        }
    }

    void ShowOneStep(long long source_id){

        for(int i=0;i<source_train.size();++i){
            if(source_train[i]==source_id || target_train[i] ==source_id){
                std::cout << id2node[source_train[i]] << "," << id2relation[relation_train[i]] << "," << id2node[target_train[i]] << std::endl;
            }
        }
    }

    std::unordered_map<std::string,int> node2count;

    void ShowRelation(long long key_relation_id){

        node2count.clear();
        for(int i=0;i<source_train.size();++i){
            if(relation_train[i]==key_relation_id){
                std::cout << id2node[source_train[i]] << "," << id2relation[key_relation_id] << "," << id2node[target_train[i]] << std::endl;
                if(node2count.find(id2node[source_train[i]])==node2count.end()){
                    node2count[id2node[source_train[i]]] = 1;
                }else{
                    node2count[id2node[source_train[i]]] += 1;
                }
                if(node2count.find(id2node[target_train[i]])==node2count.end()){
                    node2count[id2node[target_train[i]]] = 1;
                }else{
                    node2count[id2node[target_train[i]]] += 1;
                }
            }
        }
    }

    std::unordered_map<long long,int> key_id2count;

    void FindKeyID(Eigen::VectorXi &train_indices,Eigen::VectorXi &test_indices,int &max_depth,const int key_depth,const int key_order,const long long key_relation_id){
        key_id2count.clear();
        for(int i=0;i<train_indices.size();++i){
            for(int j=0;j<test_indices.size();++j){
                long long source_id = train_indices(i);
                long long target_id = test_indices(j);

                double pair = ContorPair(double(source_id),double(target_id));
                long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][key_depth][key_order];
                for(int m=0;m<indices.size();++m){
                      if(relation_train[indices[m]] == key_relation_id){

                          std::cout << id2node[source_train[indices[m]]] << "," << id2relation[key_relation_id] << "," << id2node[target_train[indices[m]]] << std::endl;

                          if(source_train[indices[m]] != source_id && source_train[indices[m]] != target_id){
                              if(key_id2count.find(source_train[indices[m]])==key_id2count.end()){
                                  key_id2count[source_train[indices[m]]] = 1;
                              }else{
                                  key_id2count[source_train[indices[m]]] += 1;
                              }
                          }
                          if(target_train[indices[m]] != source_id && target_train[indices[m]] != target_id){
                              if(key_id2count.find(target_train[indices[m]])==key_id2count.end()){
                                  key_id2count[target_train[indices[m]]] = 1;
                              }else{
                                  key_id2count[target_train[indices[m]]] += 1;
                              }
                        }
                    }
                }
            }
        }
    }

    void Predict(Eigen::VectorXi &positive_indices,Eigen::VectorXi &test_indices,std::string &method,double &alpha0,int &num_iteration,int &max_depth,int &reduce_dimension){

        std::unordered_map<long long,int> positive_indices_dict;
        for(int i=0;i<positive_indices.size();++i){
            positive_indices_dict[positive_indices(i)] = 1;}

        use_index.clear();use_index.shrink_to_fit();
        for(long long i=0;i<predict_num_nodes;++i){
            long long sample_id = predict_local2global[i];
            if(positive_indices_dict.find(sample_id)==positive_indices_dict.end()){
                use_index.push_back(i);
            }
        }
        std::unordered_map<long long,int> test_indices_dict;
        for(int i=0;i<test_indices.size();++i){// Create test_indices_dict
            test_indices_dict[test_indices(i)] = 1;}

        temp_label.clear();temp_label.shrink_to_fit();
        for(long long i=0;i<predict_num_nodes;++i){
            long long sample_id = predict_local2global[i];
            if(test_indices_dict.find(sample_id)==test_indices_dict.end()){
                temp_label.push_back(float(0.0));
            }else{
                temp_label.push_back(float(1.0));}}

        for(long long i=0;i<predict_sparse_core_matrix.outerSize();++i){// Update predict_sparse_core_matrix_temp
            for(Eigen::SparseMatrix<float, Eigen::ColMajor,long long>::InnerIterator it(predict_sparse_core_matrix,i);it;++it){
                long long source_id = predict_local2global[it.row()];
                long long target_id = predict_local2global[it.col()];
                double pair = ContorPair(double(source_id),double(target_id));
                long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                Eigen::MatrixXf middle2;
                middle2.setZero(reduce_dimension,1);
                for(int j=1;j<(max_depth+1);++j){
                    for(int k=1;k<(j+1);++k){
                        std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][j][k];
                        Eigen::MatrixXf middle1;
                        middle1.setZero(edge_matrix.cols(),1);
                        for(int l=0;l<indices.size();++l){
                            Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[l]);
                            middle1 = middle1 + temp_edge_matrix.transpose();}
                        if(middle1(0,0)!=0){
                            middle1 = middle1 / middle1.sum();}
                        Eigen::MatrixXf weight_matrix =  depth_order_weight[j][k];
                        middle2 = middle2 + weight_matrix * middle1;}
                }// end of depth order loop
                for(int l=0;l<reduce_dimension;++l){
                    //middle2(l,0) = Sigmoid(middle2(l,0));
                    middle2(l,0) = TanH(middle2(l,0));
                }
                Eigen::MatrixXf output = output_weight * middle2;
                float new_value = Sigmoid(output(0,0));
                predict_sparse_core_matrix.coeffRef(it.row(),it.col()) = new_value;
            }
        }// LOOP predict_sparse_core_matrix.outerSize()

        if(method=="sum"){
            node_scores.setZero(num_iteration,predict_num_nodes);
            for(int i=0;i<positive_indices.size();++i){
                long long place = predict_global2local[positive_indices(i)];
                node_scores(0,place) = float(1.0);}

            // Update node_scores
            for(int itr=0;itr<num_iteration-1;++itr){
                Eigen::MatrixXf node_scores_prev = node_scores.row(itr);
                for(int i=0;i<predict_num_nodes;++i){
                    Eigen::MatrixXf sliced_core_matrix = predict_sparse_core_matrix.row(i);
                    Eigen::MatrixXf new_score = sliced_core_matrix * node_scores_prev.transpose();
                    node_scores(itr+1,i) = new_score(0,0);
                }
            }
        }
    }

    std::vector<long long> train_positive_list,test_positive_list;
    std::vector<std::string> train_positive_time_list,test_positive_time_list;

    void CreateObjects(const std::string &file_dataframe,const std::string &use_label,const std::string &train_test_split_time,const std::string &test_end_split_time,const std::string &start_time,int &use_min){
        // USES core_node_id_counter,core_id2id
        std::vector<std::string> node,label2,date;
        train_positive_list.clear();train_positive_list.shrink_to_fit();
        test_positive_list.clear();test_positive_list.shrink_to_fit();
        train_positive_time_list.clear();train_positive_time_list.shrink_to_fit();
        test_positive_time_list.clear();test_positive_time_list.shrink_to_fit();
        std::ifstream input_stream(file_dataframe);
        std::string line;
        while(input_stream && std::getline(input_stream,line)){
            std::vector<std::string> v1 = Split(line,',');
            if(v1[4]>start_time){// we define our start time here
                node.push_back(v1[0]);
                label2.push_back(v1[3]);
                date.push_back(v1[4]);
            }
        }
        for(long long i=0;i<core_node_id_counter;++i){
            long long node_id = core_id2id[i];
            std::string node_name = id2node[node_id];
            std::vector<std::string> temp_date;
            int hantei_1 = 0;
            int hantei_2 = 0;
            std::string min_date,max_date;
            min_date = "9999-12-31";
            max_date = "0000-01-01";
            for(int j=0;j<node.size();++j){
                if(node[j]==node_name && label2[j]==use_label){
                    if(date[j]<=train_test_split_time){
                        hantei_1 = 1;}
                    if(date[j]>train_test_split_time && date[j] <= test_end_split_time){
                        hantei_2 = 1;}
                    if(min_date > date[j]){
                        min_date = date[j];}
                    if(max_date < date[j] && date[j] <= train_test_split_time){
                        max_date = date[j];}
                }
            }
            if(hantei_1==0 && hantei_2==1){
                test_positive_list.push_back(node_id);
                test_positive_time_list.push_back(min_date);
            }else if(hantei_1==1){
                train_positive_list.push_back(node_id);
                if(use_min==0){
                    train_positive_time_list.push_back(max_date);
                }else{
                    train_positive_time_list.push_back(min_date);
                }
            }
        }
    }

    void CreateObjectsTrim(const std::string &file_dataframe){
        // USES core_node_id_counter,core_id2id
        std::vector<std::string> node,label2,date;
        train_positive_list.clear();train_positive_list.shrink_to_fit();
        test_positive_list.clear();test_positive_list.shrink_to_fit();
        train_positive_time_list.clear();train_positive_time_list.shrink_to_fit();
        test_positive_time_list.clear();test_positive_time_list.shrink_to_fit();
        std::ifstream input_stream(file_dataframe);
        std::ofstream out_stream("/home/rh/Arbeitsraum/Files/KG/All/serial/core_nodes_djrc_adme_129002_trimmed.csv");
        std::string line;
        while(input_stream && std::getline(input_stream,line)){
            std::vector<std::string> v1 = Split(line,',');
            node.push_back(v1[0]);
            label2.push_back(v1[3]);
            date.push_back(v1[4]);
            if(node2id.find(v1[0])!=node2id.end()){
                long long node_id = node2id[v1[0]];
                if(id2label_prop_id.find(node_id)!=id2label_prop_id.end()){
                    out_stream << line << std::endl;
                }
            }
        }
    }

    void RunCommon(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU Initialize//
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        // Create local_relation_id
        relation_id2local_relation_id.clear();
        local_relation_id2relation_id.clear();

        int local_relation_id_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    for(int k=1;k<(max_depth+1);++k){
                        for(int l=1;l<(k+1);++l){
                            std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                            for(int m=0;m<indices.size();++m){
                                int place = int(relation_train[indices[m]]);
                                if(relation_id2local_relation_id.find(place)==relation_id2local_relation_id.end()){
                                    relation_id2local_relation_id[place] = local_relation_id_counter;
                                    local_relation_id2relation_id[local_relation_id_counter] = place;
                                    local_relation_id_counter += 1;}}}}}}}



        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);

                }
            }
        }

    }

    std::vector<std::string> use_label_vector;
    void ClearLabels(){
        use_label_vector.clear();
        use_label_vector.shrink_to_fit();
    }

    // For MultLP
    Eigen::MatrixXf label_matrix_mult_train,label_matrix_mult_test;
    Eigen::MatrixXi update_indices;

    void CreateObjectsMult(const std::string &file_dataframe,const std::string &use_label0,const std::string &train_test_split_time,const std::string &test_end_split_time,const std::string &start_time){// ouputs label_matrix_mult_train,label_matrix_mult_test,update_indices

        label_matrix_mult_train.resize(0,0);
        label_matrix_mult_train.setZero();
        label_matrix_mult_train.setZero(label_prop_counter,use_label_vector.size());
        label_matrix_mult_test.resize(0,0);
        label_matrix_mult_test.setZero();
        label_matrix_mult_test.setZero(label_prop_counter,use_label_vector.size());
        std::vector<int> update_indices_row,update_indices_col;
        std::vector<std::string> node,label2,date;
        std::ifstream input_stream(file_dataframe);
        std::string line;
        while(input_stream && std::getline(input_stream,line)){
            std::vector<std::string> v1 = Split(line,',');
            if(v1[4]>start_time){
                node.push_back(v1[0]);
                label2.push_back(v1[3]);
                date.push_back(v1[4]);}}
        std::unordered_map<std::string,int> use_label_map;
        for(int i=0;i<use_label_vector.size();++i) use_label_map[use_label_vector[i]] = i;
        for(long long i=0;i<label_prop_counter;++i){
            long long node_id = label_prop_id2id[i];
            std::string node_name = id2node[node_id];
            std::vector<std::string> temp_date;
            int hantei_1 = 0;
            int hantei_2 = 0;
            std::string min_date,max_date;
            min_date = "9999-12-31";
            max_date = "0000-01-01";
            std::unordered_map<std::string,std::string> min_date_map,max_date_map;
            std::unordered_map<std::string,int> hantei1_map,hantei2_map;
            for(int k=0;k<use_label_vector.size();++k){
                min_date_map[use_label_vector[k]] = min_date;
                max_date_map[use_label_vector[k]] = max_date;
                hantei1_map[use_label_vector[k]] = 0;
                hantei2_map[use_label_vector[k]] = 0;
            }
            for(int j=0;j<node.size();++j){
                if(node[j]==node_name && use_label_map.find(label2[j])!=use_label_map.end()){
                    if(date[j]<=train_test_split_time){
                        hantei1_map[label2[j]] = 1;
                    }
                    if(date[j]>train_test_split_time && date[j] <= test_end_split_time){
                        hantei2_map[label2[j]] = 1;
                    }
                    if(min_date_map[label2[j]] > date[j]){
                        min_date_map[label2[j]] = date[j];
                    }
                    if(max_date_map[label2[j]] < date[j] && date[j] <= train_test_split_time){
                        max_date_map[label2[j]] = date[j];}}}

            for(int j=0;j<use_label_vector.size();++j){
                if(hantei1_map[use_label_vector[j]]==0 && hantei2_map[use_label_vector[j]]==1){
                    int place = boost::lexical_cast<int>(id2label_prop_id[node_id]);
                    label_matrix_mult_test(place,j) = 1;
                }else if(hantei1_map[use_label_vector[j]]==1){
                    int place = boost::lexical_cast<int>(id2label_prop_id[node_id]);
                    label_matrix_mult_train(place,j) = 1;
                    update_indices_row.push_back(place);
                    update_indices_col.push_back(j);
                }
            }
        }
        update_indices.setZero(update_indices_row.size(),2);
        for(int i=0;i<update_indices_row.size();++i){
            update_indices(i,0) = update_indices_row[i];
            update_indices(i,1) = update_indices_col[i];
        }
    }

    Eigen::MatrixXf GetResult(int num_iteration){
        Eigen::MatrixXf out = Y[num_iteration];
        return(out);
    }

    std::unordered_map<int,Eigen::MatrixXf> Y;
    void MultLP(int num_iteration,const float alpha,const float lambda,const float beta,int clamping){

        for(long long k=0; k<p_matrix.outerSize(); ++k){
            for(Eigen::SparseMatrix<float, Eigen::RowMajor,long long>::InnerIterator it(p_matrix,k); it; ++it){
                long long row = it.row();
                long long col = it.col();
                float value = it.value();
                std::cout << boost::lexical_cast<std::string>(row) << "," << boost::lexical_cast<std::string>(col) << "," << boost::lexical_cast<std::string>(value) << std::endl;
            }
        }

        Eigen::SparseMatrix<float, Eigen::RowMajor,long long> p_matrix2 = beta * p_matrix;
        Y.clear();
        Eigen::MatrixXf I = lambda * Eigen::MatrixXf::Identity(label_matrix_mult_train.rows(),
                                                               label_matrix_mult_train.rows());
        Y[0] = label_matrix_mult_train;
        Y[1] = p_matrix2 * label_matrix_mult_train;
        if(clamping==1){
            for(int i=0;i<update_indices.rows();++i){
                int row = update_indices(i,0);
                int col = update_indices(i,1);
                Y[1](row,col) = label_matrix_mult_train(row,col);
            }
        }
        for(int i=2;i<num_iteration;++i){// i = 2 creates P(1)
            std::unordered_map<int,Eigen::MatrixXf> map1,map2;
            for(int j=1;j<i;++j){
                Eigen::MatrixXf Ytemp = Y[i-1];
                for(int k=j+1;k<i;++k){
                    Ytemp = p_matrix2.transpose() * Ytemp;
                }
                Ytemp = I * Ytemp;
                for(int k=j+1;k<i;++k){
                    Ytemp = p_matrix2 * Ytemp;
                }
                map1[j] = Ytemp;
            }
            for(int j=1;j<i;++j){
                Eigen::MatrixXf Ytemp = Y[i-1];
                for(int k=j;k<i;++k){
                    Ytemp = p_matrix2.transpose() * Ytemp;
                }
                Ytemp = Y[j-1].transpose() * Ytemp;
                Ytemp = Y[j-1] * Ytemp;
                Ytemp = alpha * Ytemp;
                for(int k=j;k<i;++k){
                    Ytemp = p_matrix2 * Ytemp;
                }
                map2[j] = Ytemp;
            }

            Eigen::MatrixXf Yout = Y[i-1];
            for(int j=1;j<i;++j){
                Yout = p_matrix2.transpose() * Yout;}
            Yout = p_matrix2 * Yout;
            for(int j=1;j<i;++j){
                Yout = p_matrix2 * Yout;}

            // Aggregate All
            for(auto itr=map1.begin();itr!=map1.end();++itr){
                Yout = Yout + itr->second;}
            for(auto itr=map2.begin();itr!=map2.end();++itr){
                Yout = Yout + itr->second;}
            Y[i] = Yout;// Update
            if(clamping == 1){
                for(int k=0;k<update_indices.rows();++k){
                    int row = update_indices(k,0);
                    int col = update_indices(k,1);
                    Y[i](row,col) = label_matrix_mult_train(row,col);}}
        }
    }

    std::string Convert2Path(const std::string &relation_id_string){
        std::string output_path = "";
        std::vector<long long> v1  = Split2LongVector(relation_id_string);
        for(int i=0;i<v1.size();++i){
            int relation_id = -1;
            int direction = 0;
            if(v1[i]<0){
                relation_id = -v1[i] - 1;
                direction = -1;
            }else{
                relation_id = v1[i] - 1;
                direction = 1;
            }
            if(direction == 1){
                output_path += id2relation[relation_id] + ";";
            }else{
                output_path += "Inv" + id2relation[relation_id] + ";";
            }
        }
        output_path = TrimSemi(output_path);
        return(output_path);
    }

    std::vector<long long> Convert2Relation(const std::string &relation_id_string){
        std::string output_path = "";
        std::vector<long long> v1  = Split2LongVector(relation_id_string);
        for(int i=0;i<v1.size();++i){
            int relation_id = -1;
            int direction = 0;
            if(v1[i]<0){
                relation_id = -v1[i] - 1;
                direction = -1;
            }else{
                relation_id = v1[i] - 1;
                direction = 1;
            }
            v1[i] = relation_id;
        }
        return(v1);
    }

    std::unordered_map<std::string,long long> path2count;
    std::unordered_map<long long,long long> collapsed_edge_id2num_path;
    void PathFeature(int &max_depth){
        for(auto itr=collapsed_edge_depth_path_relation.begin();itr!=collapsed_edge_depth_path_relation.end();++itr){
            long long count=0;
            for(int i=1;i<(max_depth+1);++i){
                std::unordered_map<std::string,std::vector<std::string>> path_relation = collapsed_edge_depth_path_relation[itr->first][i];
                for(auto itr2=path_relation.begin();itr2!=path_relation.end();++itr2){
                    std::vector<std::string> v1 = itr2->second;
                    for(int j=0;j<v1.size();++j){
                        //std::cout << itr2->first << "," << v1[j] << std::endl;
                        std::string path = Convert2Path(v1[j]);
                        if( path2count.find(path)==path2count.end() ){
                            path2count[path] = 1;
                        }else{
                            path2count[path] += 1;
                        }
                        count+=1;
                    }
                }
            }
            collapsed_edge_id2num_path[itr->first] = count;
        }
    }

    std::unordered_map<long long,int> relation_id2local_id;
    Eigen::MatrixXf path_matrix,path_matrix_row,path_matrix_col;
    Eigen::MatrixXf depth_matrix,depth_matrix_row,depth_matrix_col;
    Eigen::MatrixXf depth_order_matrix,depth_order_matrix_row,depth_order_matrix_col;
    Eigen::SparseMatrix<float, Eigen::ColMajor,long long> path_matrix_sparse,depth_matrix_sparse,depth_order_matrix_sparse;
    std::unordered_map<std::string,int> path2column_id;
    std::unordered_map<int,std::string> column_id2path;
    std::unordered_map<long long,float> node2degree;

    void CreateNodeDegree(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU //
        // Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        node2degree.clear();
        for(long i=0;i<label_prop_counter;++i){
            float count = 0.0;
            if(i % 1000 == 0) std::cout << i << std::endl;
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                count += 1.0;
            }
            node2degree[start_id] = count;
        }

    }

    float Specificity(const std::string &node_path){
        float specificity = 1.0;
        float temp_float = 0.0;
        std::vector<long long> v1  = Split2LongVector(node_path);
        if(v1.size()>2){
            for(int i=1;i<v1.size()-1;++i){
                temp_float += log(node2degree[v1[i]]);
            }
            specificity = 1/(1.0 + temp_float);
        }
        return(specificity);
    }

    void CreatePath2Count(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU Initialize//
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;

        path2count.clear();


        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        std::unordered_map<long long,int> collapsed_edge_id2hantei;
        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            if(i % 1000 == 0) std::cout << i << " out of " << label_prop_counter << std::endl;
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];

                    std::unordered_map<std::string,int> temp_path;


                    if(collapsed_edge_id2hantei.find(collapsed_edge_id) == collapsed_edge_id2hantei.end()){
                        // For depth matrix
                        for(int k=1;k<(max_depth+1);++k){
                            std::unordered_map<std::string,std::vector<std::string>> path_relation = collapsed_edge_depth_path_relation[collapsed_edge_id][k];
                            for(auto itr3=path_relation.begin();itr3!=path_relation.end();++itr3){
                                std::vector<std::string> v1 = itr3->second;
                                for(int j=0;j<v1.size();++j){
                                    //std::string path = Convert2Path(v1[j]);
                                    std::string path = v1[j];
                                    if(temp_path.find(path) == temp_path.end()){
                                        temp_path[path] = 1; //
                                        if( path2count.find(path)==path2count.end() ){
                                            path2count[path] = 1;
                                        }else{
                                            path2count[path] += 1;
                                        }
                                    }
                                }
                            }
                        }
                        collapsed_edge_id2hantei[collapsed_edge_id] = 1;
                    }
                }
            }
        }
        collapsed_edge_id2hantei.clear();
        /****/
    }

    void CreatePath2Count2(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU Initialize//
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;


        for(int i=0;i<train_positive.size();++i){// Train
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        for(int i=0;i<train_positive.size();++i){// Deve
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        for(int i=0;i<test_positive.size();++i){// Test
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){// Rest
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        path2count.clear();
        std::unordered_map<long long,int> collapsed_edge_id2hantei;
        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            if(i % 1000 == 0){
                std::cout << i << " out of " << label_prop_counter << std::endl;
            }
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    std::unordered_map<std::string,int> temp_path;
                    if(collapsed_edge_id2hantei.find(collapsed_edge_id) == collapsed_edge_id2hantei.end()){
                        // For depth matrix
                        for(int k=1;k<(max_depth+1);++k){
                            std::unordered_map<std::string,std::vector<std::string>> path_relation = collapsed_edge_depth_path_relation[collapsed_edge_id][k];
                            for(auto itr3=path_relation.begin();itr3!=path_relation.end();++itr3){
                                std::vector<std::string> v1 = itr3->second;
                                for(int j=0;j<v1.size();++j){

                                    // MODORU
                                    std::string path = v1[j];
                                    path = Trimquote(path);
                                    // ReturnRelationType returns collapsed relation_id
                                    // So we dont need this
                                    //std::vector<int> v2 = Split2IntVector(path);
                                    //std::string collapsed_path = "";
                                    //for(int m=0;m<v2.size();++m){
                                    //    if(v2[m] < 0){
                                    //        v2[m] = -1*v2[m];}
                                    //    if(v2[m]!=0){
                                    //        v2[m] = v2[m] - 1;}
                                    //    collapsed_path += boost::lexical_cast<std::string>(v2[m]) + ",";
                                    //}
                                    //path = TrimComma(collapsed_path);

                                    if(temp_path.find(path) == temp_path.end()){
                                        temp_path[path] = 1; //
                                        if( path2count.find(path)==path2count.end() ){
                                            path2count[path] = 1;
                                        }else{
                                            path2count[path] += 1;
                                        }
                                    }
                                }
                            }
                        }
                        collapsed_edge_id2hantei[collapsed_edge_id] = 1;
                    }
                }
            }
        }
        collapsed_edge_id2hantei.clear();
        /****/
    }

    std::unordered_map<std::string,int> include_path;
    std::vector<long long> path_row,path_col;
    std::vector<float> path_value;
    std::unordered_map<std::string,int> collapsed_path2id;
    std::unordered_map<int,std::string> id2collapsed_path;


    std::unordered_map<std::string,int> path2collapsed_path_id;

    void CreatePathMatrix3(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon,const std::string &file_path,int &path_threshold,int &zero_one){

        path2collapsed_path_id.clear();
        // Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        label_prop_counter=0;
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;
                        }
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;
                    }
                }
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                    temp_counter += 1;}}
            //// KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i){
            y_init_train(i,0) = 1.0;
        }
        for(long long i=train_number;i<label_prop_counter;++i){
            eval_indices_train.push_back(i);
        }
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i){
            y_init_test(i,0) = 1.0;
        }
        for(long long i=devel_number;i<label_prop_counter;++i){
            eval_indices_test.push_back(i);
        }
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i){
            y_full(i,0) = 1;
        }
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;

        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        label_prop_I_train.resize(0,0);
        label_prop_I_train.setZero();
        label_prop_I_train.setZero(label_prop_counter,1);
        label_prop_I_test.resize(0,0);
        label_prop_I_test.setZero();
        label_prop_I_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1 && start_id != target_id){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;
                }
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
            }

            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_I_train(i,0) = (1.0 +  mu * epsilon);
            }else{
                label_prop_I_train(i,0) = ( mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_I_test(i,0) = (1.0 +  mu * epsilon);
            }else{
                label_prop_I_test(i,0) = (mu * epsilon);
            }
        }

        // END KYOTSU //

        // NEW!
        include_path.clear();
        collapsed_path2id.clear();
        id2collapsed_path.clear();
        std::unordered_map<std::string,int> path2id;
        std::ifstream input_stream(file_path);
        std::string line;
        int line_counter = 0;
        int local_path_id_counter0 = 0;
        int local_path_id_counter = 0;


        std::unordered_map<std::string,int> path2collapsed_path_id0;
        while(input_stream && std::getline(input_stream,line)){
            if(line_counter>0){
                std::vector<std::string> v1 = SplitDqcsv(line);
                v1[0] = Trimquote(v1[0]);
                std::vector<int> v2 = Split2IntVector(v1[0]);
                std::string temp_path1;
                std::string temp_path2;
                for(int k=0;k<v2.size();++k){
                    temp_path1 += boost::lexical_cast<std::string>(v2[k]) + ",";
                    temp_path2 += boost::lexical_cast<std::string>(v2[v2.size()-1-k]) + ",";
                }
                temp_path1 = TrimComma(temp_path1);
                temp_path2 = TrimComma(temp_path2);
                if(path2collapsed_path_id0.find(temp_path1)==path2collapsed_path_id0.end() && path2collapsed_path_id0.find(temp_path2)==path2collapsed_path_id0.end()){
                    path2collapsed_path_id0[temp_path1] = local_path_id_counter0;
                    path2collapsed_path_id0[temp_path2] = local_path_id_counter0;
                    local_path_id_counter0 += 1;
                }
                if(local_path_id_counter0 >= (10000000-1)){
                    break;
                }
            }
            line_counter += 1;// Just to ignore headers
        }
        int num_path_used = 0;
        for(auto itr=path2collapsed_path_id0.begin();itr!=path2collapsed_path_id0.end();++itr){
            if(itr->second < path_threshold){
                num_path_used += 1;
                if(local_path_id_counter < itr->second){
                    local_path_id_counter = itr->second;
                }
                path2collapsed_path_id[itr->first] = itr->second;}}
        path2collapsed_path_id0.clear();
        local_path_id_counter += 1;

        /*
        while(input_stream && std::getline(input_stream,line)){
            if(line_counter>0){
                std::vector<std::string> v1 = SplitDqcsv(line);
                int count = boost::lexical_cast<int>(v1[1]);
                if(count > path_threshold){
                    v1[0] = Trimquote(v1[0]);
                    std::vector<int> v2 = Split2IntVector(v1[0]);
                    include_path[v1[0]] = local_path_id_counter;
                    local_path_id_counter += 1;
                }
            }
            line_counter += 1;// Just to ignore headers
        }
        /***/
        std::cout << "Number of path used: " << num_path_used << std::endl;


        // sparse version
        typedef Eigen::Triplet<float> T;
        std::vector<T> triplet_list;
        path_row.clear();path_row.shrink_to_fit();
        path_col.clear();path_col.shrink_to_fit();
        path_value.clear();path_value.shrink_to_fit();
        path_matrix_row.resize(0,0);
        path_matrix_row.setZero();
        path_matrix_row.setZero(num_rows,1);
        path_matrix_col.resize(0,0);
        path_matrix_col.setZero();
        path_matrix_col.setZero(1,local_path_id_counter);

        std::unordered_map<long long,Eigen::VectorXf> collapsed_edge_id2feature_vector;
        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            if(i % 1000 == 0){
                std::cout << i << " out of " << label_prop_counter << std::endl;
            }
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1 && start_id != target_id){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    if(collapsed_edge_id2feature_vector.find(collapsed_edge_id) == collapsed_edge_id2feature_vector.end()){
                        std::vector<float> tempvec;
                        Eigen::MatrixXf middle1;// For depth matrix
                        middle1.setZero(local_path_id_counter,1);
                        for(int k=1;k<(max_depth+1);++k){
                            std::unordered_map<std::string,std::vector<std::string>> path_relation = collapsed_edge_depth_path_relation[collapsed_edge_id][k];
                            for(auto itr3=path_relation.begin();itr3!=path_relation.end();++itr3){
                                std::vector<std::string> v1 = itr3->second;
                                for(int j=0;j<v1.size();++j){
                                    int place = -111;

                                    std::string path = Trimquote(v1[j]);
                                    if(path2id.find(v1[j])==path2id.end()){

                                        //std::vector<int> v2 = Split2IntVector(path);
                                        //std::string collapsed_path = "";
                                        //for(int m=0;m<v2.size();++m){
                                        //    if(v2[m] < 0){
                                        //        v2[m] = -1*v2[m];}
                                        //    if(v2[m]!=0){
                                        //        v2[m] = v2[m] - 1;}
                                        //    collapsed_path += boost::lexical_cast<std::string>(v2[m]) + ",";
                                        //}
                                        //path = TrimComma(collapsed_path);

                                        if(path2collapsed_path_id.find(path)!=path2collapsed_path_id.end()){
                                            place = path2collapsed_path_id[path];
                                            path2id[v1[j]] = place;
                                        }else{
                                            path2id[v1[j]] = -111;
                                        }
                                        //if( include_path.find(path)!=include_path.end() ){
                                        //    place = include_path[path];
                                        //    path2id[v1[j]] = place;
                                        //}else{
                                        //    path2id[v1[j]] = -111;}
                                    }else{
                                        place = path2id[v1[j]];
                                    }
                                    if( place != -111 ){
                                        Eigen::MatrixXf temp_edge_matrix;
                                        if(zero_one==1){
                                            middle1(place,0) = 1.0;
                                        }else{
                                            middle1(place,0) += 1.0;
                                        }
                                        //path_matrix_row(row_counter,0) += 1.0;
                                    }
                                }
                            }
                        }
                        if(1==1){
                            //middle1 = middle1 / middle1.sum();//KOKO HA FUYOU?
                        }
                        for(int m=0;m<local_path_id_counter;++m){
                            float value = 0.0;
                            if(zero_one!=1){
                                if(middle1(m,0)>0){
                                    // POSSIBILTY INCORRECT: COULD WE TAKE LOG for float.  if you are scared make it double first
                                    double value_d = log(double(middle1(m,0)));
                                    value = float(value_d);}
                            }else{
                                value = middle1(m,0);}
                            tempvec.push_back(value);
                        }
                        Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());

                        for(int m=0;m<local_path_id_counter;++m){// sparse version
                            if(feature_vector(m)>0.0){
                                triplet_list.push_back(T(row_counter,m,feature_vector(m)));}}
                        for(int m=0;m<local_path_id_counter;++m){
                            if(feature_vector(m)>0.0){
                                path_matrix_row(row_counter,0) += 1.0;
                                path_matrix_col(0,m) += 1.0;}}

                        collapsed_edge_id2feature_vector[collapsed_edge_id] = feature_vector;
                    }else{// Already calculated due to symmetry
                        Eigen::VectorXf feature_vector = collapsed_edge_id2feature_vector[collapsed_edge_id];

                        for(int m=0;m<local_path_id_counter;++m){// sparse version
                            if(feature_vector(m)>0.0){
                                triplet_list.push_back(T(row_counter,m,feature_vector(m)));}}
                        for(int m=0;m<local_path_id_counter;++m){
                            if(feature_vector(m)>0.0){
                                path_matrix_row(row_counter,0) += 1.0;
                                path_matrix_col(0,m) += 1.0;}}
                    }
                    row_counter += 1;
                }//if(hantei_core==1){
            }
        }
        path_matrix_sparse.resize(row_counter,local_path_id_counter);
        path_matrix_sparse.setFromTriplets(triplet_list.begin(),triplet_list.end());
        collapsed_edge_id2feature_vector.clear();
    }

    std::vector<std::string> label_nodes;
    std::unordered_map<std::string,int> label_nodes_dict;

    void ClearLabelNodes(){
        label_nodes.clear();
        label_nodes.shrink_to_fit();
    }

    void InitializeJumpCoreMatrix(){
        // PREPARE: label_nodes
        // Initialize
        id2core_id.clear();
        core_id2id.clear();
        double count_edges_core_matrix = 0;
        core_node_id_counter = 0;
        for(int i=0;i<label_nodes.size();++i) label_nodes_dict[label_nodes[i]] = 1;
        for(auto itr=label_nodes_dict.begin();itr!=label_nodes_dict.end();++itr){
            if(node2id.find(itr->first)==node2id.end()){
                // We have no relational information forget it
            }else{
                long long source_id =node2id.at(itr->first);
                if(id2core_id.find(source_id)==id2core_id.end()){
                    id2core_id[source_id] = core_node_id_counter;
                    core_id2id[core_node_id_counter] = source_id;
                    core_node_id_counter += 1;}}}
    }

    //Eigen::SparseMatrix<float, Eigen::ColMajor,long long> jump_matrix_sparse;
    Eigen::SparseMatrix<float, Eigen::RowMajor,long long> jump_matrix_sparse;
    Eigen::SparseMatrix<float, Eigen::ColMajor,long long> jump_feature_sparse;

    void CreateJumpMatrix(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon,const std::string &file_path,int &path_threshold){

        // Need to build label_nodes


        //std::ofstream ofs(output_file);
        std::ofstream ofs("jump_feature.csv");

        // START KYOTSU INITIALIZE //
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        include_path.clear();
        std::ifstream input_stream(file_path);
        std::string line;
        int line_counter = 0;
        int local_path_id_counter = 0;
        while(input_stream && std::getline(input_stream,line)){
            if(line_counter>0){
                std::vector<std::string> v1 = SplitDqcsv(line);
                int count = boost::lexical_cast<int>(v1[1]);
                if(count > path_threshold){
                    v1[0] = Trimquote(v1[0]);
                    include_path[v1[0]] = local_path_id_counter;
                    local_path_id_counter += 1;}}
            line_counter += 1;}
        std::cout << "Number of path used: " << local_path_id_counter << std::endl;

        std::unordered_map<std::string,Eigen::MatrixXf> collapsed_edge2feature;
        std::unordered_map<long long,int> reachable_nodes_dict;
        for(int i=0;i<label_nodes.size();++i){
            if(i % 1000==0) std::cout << "Finished: " << i << " out of " << label_nodes.size() << std::endl;
            int reachable = 0;
            std::unordered_map<long long,int> nodes_visited;
            long long start_id = node2id[label_nodes[i]];
            nodes_visited[start_id] = 1;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr=range.first;itr!=range.second;++itr){
                long long target_id = itr->second;
                std::string target_word = id2node[target_id];
                std::vector<long long> depth1 = ReturnRelationType(start_id,target_id);
                if(label_nodes_dict.find(target_word)!=label_nodes_dict.end()){
                //if(core_nodes_dict.find(target_id)!=core_nodes_dict.end()){
                    reachable = 1;
                    std::string pair;
                    if(start_id < target_id){
                        pair = boost::lexical_cast<std::string>(start_id) + "," + boost::lexical_cast<std::string>(target_id);
                    }else{
                        pair = boost::lexical_cast<std::string>(target_id) + "," + boost::lexical_cast<std::string>(start_id);}
                    Eigen::MatrixXf middle1;
                    if(collapsed_edge2feature.find(pair)==collapsed_edge2feature.end()){
                        middle1.setZero(local_path_id_counter,1);
                    }else{
                        middle1 = collapsed_edge2feature[pair];}
                    // Naniwo Tadoltuta? //PRA nohoude Tadoltuta mono mo ireru
                    for(int j=0;j<depth1.size();++j){
                        std::string path = boost::lexical_cast<std::string>(depth1[j]);
                        if(include_path.find(path)!=include_path.end()){
                            int place = include_path[path];
                            middle1(place,0) += 1.0;
                            //path_matrix_row(row_counter,0) += 1.0;
                        }
                    }
                    collapsed_edge2feature[pair] = middle1;
                }
                if(nodes_visited.find(target_id)==nodes_visited.end()){//depth 2
                    nodes_visited[target_id] = 1;
                    auto range2 = edgelist_collapsed_multimap_train.equal_range(target_id);
                    for(auto itr2=range2.first;itr2!=range2.second;++itr2){
                        long long target_id2 = itr2->second;
                        std::string target_word2 = id2node[target_id2];
                        std::vector<long long> depth2 = ReturnRelationType(target_id,target_id2);
                        if(label_nodes_dict.find(target_word2)!=label_nodes_dict.end()){
                        //if(core_nodes_dict.find(target_id2)!=core_nodes_dict.end()){
                            reachable = 1;
                            std::string pair2;
                            if(target_id < target_id2){
                                pair2 = boost::lexical_cast<std::string>(target_id) + "," + boost::lexical_cast<std::string>(target_id2);
                            }else{
                                pair2 = boost::lexical_cast<std::string>(target_id2) + "," + boost::lexical_cast<std::string>(target_id);}
                            Eigen::MatrixXf middle2;
                            if(collapsed_edge2feature.find(pair2)==collapsed_edge2feature.end()){
                                middle2.setZero(local_path_id_counter,1);
                            }else{
                                middle2 = collapsed_edge2feature[pair2];}
                            // Naniwo Tadoltuta? //PRA nohoude Tadoltuta mono mo ireru
                            for(int j=0;j<depth1.size();++j){
                                for(int k=0;k<depth2.size();++k){
                                    std::string path2 = boost::lexical_cast<std::string>(depth1[j]) + "," + boost::lexical_cast<std::string>(depth2[k]);
                                    if(include_path.find(path2)==include_path.end()){
                                        int place = include_path[path2];
                                        middle2(place,0) += 1.0;
                                        //path_matrix_row(row_counter,0) += 1.0;
                                    }
                                }
                            }
                            collapsed_edge2feature[pair2] = middle2;
                        }
                        if(nodes_visited.find(target_id2)==nodes_visited.end()){
                            nodes_visited[target_id2] = 1;
                            auto range3 = edgelist_collapsed_multimap_train.equal_range(target_id2);
                            for(auto itr3=range3.first;itr3!=range3.second;++itr3){
                                long long target_id3 = itr3->second;
                                std::string target_word3 = id2node[target_id3];
                                std::vector<long long> depth3 = ReturnRelationType(target_id2,target_id3);
                                if(label_nodes_dict.find(target_word3)!=label_nodes_dict.end()){
                                //if(core_nodes_dict.find(target_id3)!=core_nodes_dict.end()){
                                    reachable = 1;
                                    std::string pair3;
                                    if(target_id2 < target_id3){
                                        pair3 = boost::lexical_cast<std::string>(target_id2) + "," + boost::lexical_cast<std::string>(target_id3);
                                    }else{
                                        pair3 = boost::lexical_cast<std::string>(target_id3) + "," + boost::lexical_cast<std::string>(target_id2);}
                                    Eigen::MatrixXf middle3;
                                    if(collapsed_edge2feature.find(pair3)==collapsed_edge2feature.end()){
                                        middle3.setZero(local_path_id_counter,1);
                                    }else{
                                        middle3 = collapsed_edge2feature[pair3];}
                                    // Naniwo Tadoltuta? //PRA nohoude Tadoltuta mono mo ireru
                                    for(int j=0;j<depth1.size();++j){
                                        for(int k=0;k<depth2.size();++k){
                                            for(int l=0;l<depth3.size();++l){
                                                std::string path3 = boost::lexical_cast<std::string>(depth1[j]) + "," + boost::lexical_cast<std::string>(depth2[k]) + "," + boost::lexical_cast<std::string>(depth3[l]);
                                                if(include_path.find(path3)==include_path.end()){
                                                    int place = include_path[path3];
                                                    middle3(place,0) += 1.0;
                                                    //path_matrix_row(row_counter,0) += 1.0;
                                                }
                                            }
                                        }
                                    }
                                    collapsed_edge2feature[pair3] = middle3;
                                }
                            }
                        }
                    }
                }
            }
            if(reachable==1){
                reachable_nodes_dict[start_id] = 1;
            }
        }

        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_counter=0;
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        // Train
        for(int i=0;i<train_positive.size();++i){
            long long start_id = train_positive(i);
            float temp_counter = 0;
            if(reachable_nodes_dict.find(start_id)!=reachable_nodes_dict.end()){// KOKO
                if(train_positive_time[i] < train_deve_split){
                    train_positive_dict[train_positive(i)] = train_positive_time[i];
                    if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[train_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = train_positive(i);
                        label_prop_counter += 1;}
                }else{
                    devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            long long start_id = train_positive(i);
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            if(reachable_nodes_dict.find(start_id)!=reachable_nodes_dict.end()){
                if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                    if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[train_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = train_positive(i);
                        label_prop_counter += 1;}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            long long start_id = test_positive(i);
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            if(reachable_nodes_dict.find(start_id)!=reachable_nodes_dict.end()){
                test_positive_dict[test_positive(i)] = "1";
                if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                    id2label_prop_id[test_positive(i)] = label_prop_counter;
                    label_prop_id2id[label_prop_counter] = test_positive(i);
                    label_prop_counter += 1;}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=reachable_nodes_dict.begin();itr!=reachable_nodes_dict.end();++itr){
            long long start_id = itr->first;
            if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                    id2label_prop_id[itr->first] = label_prop_counter;
                    label_prop_id2id[label_prop_counter] = itr->first;
                    label_prop_counter += 1;}}}

        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);

        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                //std::string source_word = id2node[start_id];
                //std::string target_word = id2node[target_id];
                //if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()) hantei_core = 1;
                if(reachable_nodes_dict.find(start_id)!=reachable_nodes_dict.end() && reachable_nodes_dict.find(target_id)!=reachable_nodes_dict.end()) hantei_core = 1;
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}

        typedef Eigen::Triplet<float> T;
        std::vector<T> triplet_list,triplet_list2;

        long long row_counter = 0;
        for(auto itr=collapsed_edge2feature.begin();itr!=collapsed_edge2feature.end();++itr){
            std::vector<long long> v1 = Split2LongVector(itr->first);
            long long node_a = id2label_prop_id[v1[0]];
            long long node_b = id2label_prop_id[v1[1]];
            std::string pair;
            if(node_a < node_b){
                pair = boost::lexical_cast<std::string>(node_a) + "," + boost::lexical_cast<std::string>(node_b);
            }else{
                pair = boost::lexical_cast<std::string>(node_b) + "," + boost::lexical_cast<std::string>(node_a);}
            triplet_list.push_back(T(node_a,node_b,float(1.0)));
            triplet_list.push_back(T(node_b,node_a,float(1.0)));
        }

        jump_matrix_sparse.resize(num_rows,num_rows);
        jump_matrix_sparse.setFromTriplets(triplet_list.begin(),triplet_list.end());

        row_counter = 0;
        for(int k=0;k<jump_matrix_sparse.outerSize();++k){
            if(k % 1000==0) std::cout << "Finished: " << k << " out of " << jump_matrix_sparse.outerSize() << std::endl;
            //for(Eigen::SparseMatrix<float, Eigen::ColMajor,long long>::InnerIterator it(jump_matrix_sparse,k);it;++it){
            for(Eigen::SparseMatrix<float, Eigen::RowMajor,long long>::InnerIterator it(jump_matrix_sparse,k);it;++it){
                std::string pair;
                if(it.row() < it.col()){
                    pair = boost::lexical_cast<std::string>(it.row()) + "," + boost::lexical_cast<std::string>(it.col());
                }else{
                    pair = boost::lexical_cast<std::string>(it.col()) + "," + boost::lexical_cast<std::string>(it.row());}
                // Kono Edge ni Taishite
                label_prop_row.push_back(it.row());
                label_prop_col.push_back(it.col());
                Eigen::MatrixXf middle = collapsed_edge2feature[pair];
                for(int l=0;l<local_path_id_counter;++l){
                    if(middle(l,0)>0){
                        //triplet_list2.push_back(T(row_counter,l,float(1.0)));
                        ofs << row_counter << "," << l << "," << middle(l,0) << std::endl;
                }}
                row_counter += 1;
            }
        }
        //jump_feature_sparse.resize(row_counter,local_path_id_counter);
        //jump_feature_sparse.setFromTriplets(triplet_list2.begin(),triplet_list2.end());
        collapsed_edge2feature.clear();
        /****/
    }

    std::vector<float> specificity;

    void CreateSpecificity(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();
        label_prop_counter=0;
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();
        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        // Create local_relation_id
        relation_id2local_relation_id.clear();
        local_relation_id2relation_id.clear();
        int local_relation_id_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    for(int k=1;k<(max_depth+1);++k){
                        for(int l=1;l<(k+1);++l){
                            std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                            for(int m=0;m<indices.size();++m){
                                int place = int(relation_train[indices[m]]);
                                if(relation_id2local_relation_id.find(place)==relation_id2local_relation_id.end()){
                                    relation_id2local_relation_id[place] = local_relation_id_counter;
                                    local_relation_id2relation_id[local_relation_id_counter] = place;
                                    local_relation_id_counter += 1;}}}}}}}
        // KOKO KARA KAWARU
        // label_prop_weight.setZero(num_rows,num_matrix*edge_matrix.cols());
        // depth
        depth_matrix.resize(0,0);
        depth_matrix.setZero();
        depth_matrix.setZero(num_rows,max_depth*local_relation_id_counter);
        depth_matrix_row.resize(0,0);
        depth_matrix_row.setZero();
        depth_matrix_row.setZero(num_rows,1);
        depth_matrix_col.resize(0,0);
        depth_matrix_col.setZero();
        depth_matrix_col.setZero(1,max_depth*local_relation_id_counter);
        // depth order
        depth_order_matrix.resize(0,0);
        depth_order_matrix.setZero();
        depth_order_matrix.setZero(num_rows,num_matrix*local_relation_id_counter);
        depth_order_matrix_row.resize(0,0);
        depth_order_matrix_row.setZero();
        depth_order_matrix_row.setZero(num_rows,1);
        depth_order_matrix_col.resize(0,0);
        depth_order_matrix_col.setZero();
        depth_order_matrix_col.setZero(1,num_matrix*local_relation_id_counter);

        long long row_counter = 0;
        int num_in = 0;
        int num_out = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    std::vector<float> tempvec,tempvec2;
                    for(int k=1;k<(max_depth+1);++k){
                        // For depth matrix
                        Eigen::MatrixXf middle2;
                        middle2.setZero(local_relation_id_counter,1);
                        if(k==1){
                            specificity.push_back(1.0);
                        }else{
                            std::unordered_map<long long,int> temp_map;
                            temp_map[start_id] = 1;
                            temp_map[target_id] = 1;
                            for(int l=1;l<(k+1);++l){
                                // For depth order matrix
                                Eigen::MatrixXf middle1;
                                middle1.setZero(local_relation_id_counter,1);
                                std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                                for(int m=0;m<indices.size();++m){
                                    long long targ_indice = 0;
                                    if(temp_map.find(source_train[indices[m]])==temp_map.end()) targ_indice = source_train[indices[m]];
                                    if(temp_map.find(target_train[indices[m]])==temp_map.end()) targ_indice = target_train[indices[m]];
                                    float val = float(1.0) / (float(1.0) + log(node2degree[targ_indice]));
                                    if(val > 0.1){
                                        //Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[m]);
                                        Eigen::MatrixXf temp_edge_matrix;
                                        temp_edge_matrix.setZero(local_relation_id_counter,1);
                                        int place = relation_id2local_relation_id[int(relation_train[indices[m]])];
                                        temp_edge_matrix(place,0) = 1.0;
                                        middle1 = middle1 + temp_edge_matrix;// For depth order matrix
                                        middle2 = middle2 + temp_edge_matrix;// For depth matrix
                                        num_in += 1;
                                    }else{
                                        num_out += 1;
                                    }
                                }
                                if(indices.size()>0){//KOKO HA FUYOU?
                                    //middle1 = middle1 / middle1.sum();
                                }
                                for(int m=0;m<local_relation_id_counter;++m){
                                    tempvec.push_back(middle1(m,0));// For depth order matrix
                                }
                            }
                            for(int m=0;m<local_relation_id_counter;++m){
                                tempvec2.push_back(middle2(m,0));
                            }
                        }// end of depth order loop
                        //label_prop_weight.row(row_counter) = feature_vector;
                        Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());
                        depth_order_matrix.row(row_counter) = feature_vector;
                        depth_order_matrix_row(row_counter,0) = feature_vector.sum();
                        for(int m = 0;m<tempvec.size();++m){
                            if(feature_vector(m)>0){
                                depth_order_matrix_col(0,m) += 1.0;}}

                        Eigen::VectorXf feature_vector2 = Eigen::Map<Eigen::VectorXf>(&tempvec2[0],tempvec2.size());
                        depth_matrix.row(row_counter) = feature_vector2;
                        depth_matrix_row(row_counter,0) = feature_vector2.sum();
                        for(int m = 0;m<tempvec2.size();++m){
                            if(feature_vector2(m)>0){
                                depth_matrix_col(0,m) += 1.0;}}
                        row_counter += 1;
                    }
                }
            }
        }
        std::cout << "Num In: " << num_in << " Num Out: " << num_out << std::endl;
    }

    std::unordered_map<int,int> local_relation_id2relation_id,relation_id2local_relation_id;

    void CreateDepthMatrix(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){
        // START KYOTSU Initialize
        int num_matrix = 0,counter = 0;
        while(counter < max_depth){
            num_matrix = num_matrix + counter + 1;
            ++counter;}
        std::unordered_map<long long,std::string> train_positive_dict;
        std::unordered_map<long long,std::string> devel_positive_dict;
        std::unordered_map<long long,std::string> test_positive_dict;
        label_prop_id2id.clear();
        id2label_prop_id.clear();


        typedef Eigen::Triplet<float> T;
        std::vector<T> triplet_list;

        label_prop_counter=0;
        label_prop_row.clear();label_prop_row.shrink_to_fit();
        label_prop_col.clear();label_prop_col.shrink_to_fit();

        // Train
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                float temp_counter = 0;
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(train_positive_time[i] < train_deve_split){
                        train_positive_dict[train_positive(i)] = train_positive_time[i];
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}
                    }else{
                        devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
        train_number = label_prop_counter;
        // Deve
        for(int i=0;i<train_positive.size();++i){
            if(id2core_id.find(train_positive(i))!=id2core_id.end()){
                long long start_id = train_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    int hantei_core = 0;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                        if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                            id2label_prop_id[train_positive(i)] = label_prop_counter;
                            label_prop_id2id[label_prop_counter] = train_positive(i);
                            label_prop_counter += 1;}}}}}
        devel_number = label_prop_counter;
        // Test
        for(int i=0;i<test_positive.size();++i){
            if(id2core_id.find(test_positive(i))!=id2core_id.end()){
                long long start_id = test_positive(i);
                auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
                float temp_counter = 0;
                for(auto itr2=range.first;itr2!=range.second;++itr2){
                    long long target_id = itr2->second;
                    std::string source_word = id2node[start_id];
                    std::string target_word = id2node[target_id];
                    if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                        temp_counter += 1;}}
                // KOKO
                if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                    test_positive_dict[test_positive(i)] = "1";
                    if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                        id2label_prop_id[test_positive(i)] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = test_positive(i);
                        label_prop_counter += 1;}}}}
        test_number = label_prop_counter;
        // Rest
        for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
            long long start_id = itr->first;
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    temp_counter += 1;}}
            // KOKO
            if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                    if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                        id2label_prop_id[itr->first] = label_prop_counter;
                        label_prop_id2id[label_prop_counter] = itr->first;
                        label_prop_counter += 1;}}}}
        eval_indices_train.clear();
        eval_indices_train.shrink_to_fit();
        eval_indices_test.clear();
        eval_indices_test.shrink_to_fit();
        y_init_train.resize(0,0);
        y_init_train.setZero();
        y_init_train.setZero(label_prop_counter,1);
        for(long long i=0;i<train_number;++i) y_init_train(i,0) = 1.0;
        for(long long i=train_number;i<label_prop_counter;++i) eval_indices_train.push_back(i);
        y_init_test.resize(0,0);
        y_init_test.setZero();
        y_init_test.setZero(label_prop_counter,1);
        for(long long i=0;i<devel_number;++i) y_init_test(i,0) = 1.0;
        for(long long i=devel_number;i<label_prop_counter;++i) eval_indices_test.push_back(i);
        y_full.resize(0,0);
        y_full.setZero();
        y_full.setZero(label_prop_counter,1);
        for(long long i=0;i<test_number;++i) y_full(i,0) = 1;
        std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;
        label_prop_inverse_train.resize(0,0);
        label_prop_inverse_train.setZero();
        label_prop_inverse_train.setZero(label_prop_counter,1);
        label_prop_inverse_test.resize(0,0);
        label_prop_inverse_test.setZero();
        label_prop_inverse_test.setZero(label_prop_counter,1);
        long long num_rows = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            float temp_counter = 0;
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;
                }
                if(hantei_core==1){
                    temp_counter += 1;
                    long long local_target = id2label_prop_id[target_id];
                    num_rows += 1;}}
            if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
                label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_train(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);
            }
            if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
                label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
            }else{
                label_prop_inverse_test(i,0) = 1.0 / ( mu*temp_counter + mu * epsilon);}}
        // END KYOTSU //

        // Create local_relation_id
        relation_id2local_relation_id.clear();
        local_relation_id2relation_id.clear();

        int local_relation_id_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    for(int k=1;k<(max_depth+1);++k){
                        for(int l=1;l<(k+1);++l){
                            std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                            for(int m=0;m<indices.size();++m){
                                int place = int(relation_train[indices[m]]);
                                if(relation_id2local_relation_id.find(place)==relation_id2local_relation_id.end()){
                                    relation_id2local_relation_id[place] = local_relation_id_counter;
                                    local_relation_id2relation_id[local_relation_id_counter] = place;
                                    local_relation_id_counter += 1;}}}}}}}

        // KOKO KARA KAWARU
        // label_prop_weight.setZero(num_rows,num_matrix*edge_matrix.cols());
        // depth
        depth_matrix.resize(0,0);
        depth_matrix.setZero();
        depth_matrix.setZero(num_rows,max_depth*local_relation_id_counter);
        depth_matrix_row.resize(0,0);
        depth_matrix_row.setZero();
        depth_matrix_row.setZero(num_rows,1);
        depth_matrix_col.resize(0,0);
        depth_matrix_col.setZero();
        depth_matrix_col.setZero(1,max_depth*local_relation_id_counter);

        // depth order
        depth_order_matrix.resize(0,0);
        depth_order_matrix.setZero();
        depth_order_matrix.setZero(num_rows,num_matrix*local_relation_id_counter);
        depth_order_matrix_row.resize(0,0);
        depth_order_matrix_row.setZero();
        depth_order_matrix_row.setZero(num_rows,1);
        depth_order_matrix_col.resize(0,0);
        depth_order_matrix_col.setZero();
        depth_order_matrix_col.setZero(1,num_matrix*local_relation_id_counter);

        long long row_counter = 0;
        for(long i=0;i<label_prop_counter;++i){
            long long start_id = label_prop_id2id[i];
            // start_id has to be GLOBAL ID
            auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
            for(auto itr2=range.first;itr2!=range.second;++itr2){
                long long target_id = itr2->second;
                int hantei_core = 0;
                std::string source_word = id2node[start_id];
                std::string target_word = id2node[target_id];
                if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                    hantei_core = 1;}
                if(hantei_core==1){
                    long long local_target = id2label_prop_id[target_id];
                    label_prop_row.push_back(i);
                    label_prop_col.push_back(local_target);
                    triplet_list.push_back(T(i,local_target,float(1.0)));
                    double pair = ContorPair(double(start_id),double(target_id));
                    long long collapsed_edge_id = pair2collapsed_edge_id[pair];
                    std::vector<float> tempvec,tempvec2;
                    for(int k=1;k<(max_depth+1);++k){
                        // For depth matrix
                        Eigen::MatrixXf middle2;
                        middle2.setZero(local_relation_id_counter,1);
                        for(int l=1;l<(k+1);++l){
                            // For depth order matrix
                            Eigen::MatrixXf middle1;
                            middle1.setZero(local_relation_id_counter,1);
                            std::vector<long long> indices = collapsed_edge_depth_order_backpath[collapsed_edge_id][k][l];
                            for(int m=0;m<indices.size();++m){
                                //Eigen::MatrixXf temp_edge_matrix = edge_matrix.row(indices[m]);
                                Eigen::MatrixXf temp_edge_matrix;
                                temp_edge_matrix.setZero(local_relation_id_counter,1);
                                int place = relation_id2local_relation_id[int(relation_train[indices[m]])];
                                temp_edge_matrix(place,0) = 1.0;
                                middle1 = middle1 + temp_edge_matrix;// For depth order matrix
                                middle2 = middle2 + temp_edge_matrix;// For depth matrix
                            }
                            if(indices.size()>0){//KOKO HA FUYOU?
                                //middle1 = middle1 / middle1.sum();
                            }
                            for(int m=0;m<local_relation_id_counter;++m){
                                tempvec.push_back(middle1(m,0));// For depth order matrix
                            }
                        }
                        for(int m=0;m<local_relation_id_counter;++m){
                            tempvec2.push_back(middle2(m,0));
                        }
                    }// end of depth order loop
                    //label_prop_weight.row(row_counter) = feature_vector;
                    Eigen::VectorXf feature_vector = Eigen::Map<Eigen::VectorXf>(&tempvec[0],tempvec.size());
                    depth_order_matrix.row(row_counter) = feature_vector;
                    depth_order_matrix_row(row_counter,0) = feature_vector.sum();
                    for(int m = 0;m<tempvec.size();++m){
                        if(feature_vector(m)>0){
                            depth_order_matrix_col(0,m) += 1.0;}}

                    Eigen::VectorXf feature_vector2 = Eigen::Map<Eigen::VectorXf>(&tempvec2[0],tempvec2.size());
                    depth_matrix.row(row_counter) = feature_vector2;
                    depth_matrix_row(row_counter,0) = feature_vector2.sum();
                    for(int m = 0;m<tempvec2.size();++m){
                        if(feature_vector2(m)>0){
                            depth_matrix_col(0,m) += 1.0;}}
                    row_counter += 1;
                }
            }
        }
        // ADDED //
        sparse_core_matrix.resize(0,0);
        sparse_core_matrix.setZero();
        sparse_core_matrix.data().squeeze();
        sparse_core_matrix.resize(label_prop_counter,label_prop_counter);
        sparse_core_matrix.setFromTriplets(triplet_list.begin(), triplet_list.end());
        for(long long i=0;i<label_prop_counter;++i){
            //float total = sparse_core_matrix.row(i).sum();
            //sparse_core_matrix.row(i) = sparse_core_matrix.row(i) / total;
            float total = sparse_core_matrix.col(i).sum();
            sparse_core_matrix.col(i) = sparse_core_matrix.col(i) / total;
        }
    }

    void CreateMultMatrix(Eigen::VectorXi &train_positive,std::string &train_deve_split,Eigen::VectorXi &test_positive,float &num_edge_threshold,int &max_depth,int &reduce_dimension,float &mu,float &epsilon){

      // Initialize
      int num_matrix = 0,counter = 0;
      while(counter < max_depth){
          num_matrix = num_matrix + counter + 1;
          ++counter;}
      std::unordered_map<long long,std::string> train_positive_dict;
      std::unordered_map<long long,std::string> devel_positive_dict;
      std::unordered_map<long long,std::string> test_positive_dict;
      label_prop_id2id.clear();
      id2label_prop_id.clear();
      label_prop_row.clear();label_prop_row.shrink_to_fit();
      label_prop_col.clear();label_prop_col.shrink_to_fit();
      label_prop_counter=0;
      // Train
      for(int i=0;i<train_positive.size();++i){
          if(id2core_id.find(train_positive(i))!=id2core_id.end()){
              long long start_id = train_positive(i);
              float temp_counter = 0;
              auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
              for(auto itr2=range.first;itr2!=range.second;++itr2){
                  long long target_id = itr2->second;
                  int hantei_core = 0;
                  std::string source_word = id2node[start_id];
                  std::string target_word = id2node[target_id];
                  if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                      temp_counter += 1;}}
              // KOKO
              if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                  if(train_positive_time[i] < train_deve_split){
                      train_positive_dict[train_positive(i)] = train_positive_time[i];
                      if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                          id2label_prop_id[train_positive(i)] = label_prop_counter;
                          label_prop_id2id[label_prop_counter] = train_positive(i);
                          label_prop_counter += 1;
                      }
                  }else{
                      devel_positive_dict[train_positive(i)] = train_positive_time[i];}}}}
      train_number = label_prop_counter;
      // Deve
      for(int i=0;i<train_positive.size();++i){
          if(id2core_id.find(train_positive(i))!=id2core_id.end()){
              long long start_id = train_positive(i);
              auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
              float temp_counter = 0;
              for(auto itr2=range.first;itr2!=range.second;++itr2){
                  long long target_id = itr2->second;
                  int hantei_core = 0;
                  std::string source_word = id2node[start_id];
                  std::string target_word = id2node[target_id];
                  if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                      temp_counter += 1;
                  }
              }
              // KOKO
              if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                  if(devel_positive_dict.find(train_positive(i))!=devel_positive_dict.end()){
                      if(id2label_prop_id.find(train_positive(i)) == id2label_prop_id.end()){
                          id2label_prop_id[train_positive(i)] = label_prop_counter;
                          label_prop_id2id[label_prop_counter] = train_positive(i);
                          label_prop_counter += 1;}}}}}
      devel_number = label_prop_counter;
      // Test
      for(int i=0;i<test_positive.size();++i){
          if(id2core_id.find(test_positive(i))!=id2core_id.end()){
              long long start_id = test_positive(i);
              auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
              float temp_counter = 0;
              for(auto itr2=range.first;itr2!=range.second;++itr2){
                  long long target_id = itr2->second;
                  std::string source_word = id2node[start_id];
                  std::string target_word = id2node[target_id];
                  if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                      temp_counter += 1;}}
              // KOKO
              if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
                  test_positive_dict[test_positive(i)] = "1";
                  if(id2label_prop_id.find(test_positive(i)) == id2label_prop_id.end()){
                      id2label_prop_id[test_positive(i)] = label_prop_counter;
                      label_prop_id2id[label_prop_counter] = test_positive(i);
                      label_prop_counter += 1;}}}}
      test_number = label_prop_counter;
      // Rest
      for(auto itr=id2core_id.begin();itr!=id2core_id.end();++itr){
          long long start_id = itr->first;
          auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
          float temp_counter = 0;
          for(auto itr2=range.first;itr2!=range.second;++itr2){
              long long target_id = itr2->second;
              std::string source_word = id2node[start_id];
              std::string target_word = id2node[target_id];
              if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end() && start_id != target_id){
                  temp_counter += 1;}}
          //// KOKO
          if(temp_counter>(num_edge_threshold-1)){// Prune Core Edge
              if(train_positive_dict.find(itr->first)==train_positive_dict.end() && devel_positive_dict.find(itr->first)==devel_positive_dict.end() && test_positive_dict.find(itr->first)==test_positive_dict.end()){
                  if(id2label_prop_id.find(itr->first) == id2label_prop_id.end()){
                      id2label_prop_id[itr->first] = label_prop_counter;
                      label_prop_id2id[label_prop_counter] = itr->first;
                      label_prop_counter += 1;}}}}
      eval_indices_train.clear();
      eval_indices_train.shrink_to_fit();
      eval_indices_test.clear();
      eval_indices_test.shrink_to_fit();
      y_init_train.resize(0,0);
      y_init_train.setZero();
      y_init_train.setZero(label_prop_counter,1);
      for(long long i=0;i<train_number;++i){
          y_init_train(i,0) = 1.0;
      }
      for(long long i=train_number;i<label_prop_counter;++i){
          eval_indices_train.push_back(i);
      }
      y_init_test.resize(0,0);
      y_init_test.setZero();
      y_init_test.setZero(label_prop_counter,1);
      for(long long i=0;i<devel_number;++i){
          y_init_test(i,0) = 1.0;
      }
      for(long long i=devel_number;i<label_prop_counter;++i){
          eval_indices_test.push_back(i);
      }
      y_full.resize(0,0);
      y_full.setZero();
      y_full.setZero(label_prop_counter,1);
      for(long long i=0;i<test_number;++i){
          y_full(i,0) = 1;
      }
      std::cout << "Number label_prop_counter: " << label_prop_counter << std::endl;



      label_prop_inverse_train.resize(0,0);
      label_prop_inverse_train.setZero();
      label_prop_inverse_train.setZero(label_prop_counter,1);
      label_prop_inverse_test.resize(0,0);
      label_prop_inverse_test.setZero();
      label_prop_inverse_test.setZero(label_prop_counter,1);

      label_prop_I_train.resize(0,0);
      label_prop_I_train.setZero();
      label_prop_I_train.setZero(label_prop_counter,1);
      label_prop_I_test.resize(0,0);
      label_prop_I_test.setZero();
      label_prop_I_test.setZero(label_prop_counter,1);
      long long num_rows = 0;
      for(long i=0;i<label_prop_counter;++i){
          long long start_id = label_prop_id2id[i];
          // start_id has to be GLOBAL ID
          auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
          float temp_counter = 0;
          for(auto itr2=range.first;itr2!=range.second;++itr2){
              long long target_id = itr2->second;
              int hantei_core = 0;
              std::string source_word = id2node[start_id];
              std::string target_word = id2node[target_id];
              if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                  hantei_core = 1;
              }
              if(hantei_core==1 && start_id != target_id){
                  temp_counter += 1;
                  long long local_target = id2label_prop_id[target_id];
                  num_rows += 1;
              }
          }
          if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
              label_prop_inverse_train(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
          }else{
              label_prop_inverse_train(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
          }
          if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
              label_prop_inverse_test(i,0) = 1.0 / (1.0 + mu*temp_counter + mu * epsilon);
          }else{
              label_prop_inverse_test(i,0) = 1.0 / (mu*temp_counter + mu * epsilon);
          }

          if(train_positive_dict.find(start_id)!=train_positive_dict.end()){
              label_prop_I_train(i,0) = (1.0 +  mu * epsilon);
          }else{
              label_prop_I_train(i,0) = ( mu * epsilon);
          }
          if(train_positive_dict.find(start_id)!=train_positive_dict.end() || devel_positive_dict.find(start_id)!=devel_positive_dict.end()){
              label_prop_I_test(i,0) = (1.0 +  mu * epsilon);
          }else{
              label_prop_I_test(i,0) = (mu * epsilon);
          }
      }

      //label_prop_weight.setZero(num_rows,num_matrix*edge_matrix.cols());
      label_prop_weight.resize(0,0);
      label_prop_weight.setZero();
      //label_prop_weight.setZero(num_rows,num_matrix*relation_id_counter);
      label_prop_weight.setZero(num_rows,6*relation_id_counter);

      typedef Eigen::Triplet<float> T;
      std::vector<T> triplet_list;

      long long row_counter = 0;
      for(long i=0;i<label_prop_counter;++i){
          long long start_id = label_prop_id2id[i];
          // start_id has to be GLOBAL ID
          auto range = edgelist_collapsed_multimap_train.equal_range(start_id);
          for(auto itr2=range.first;itr2!=range.second;++itr2){
              long long target_id = itr2->second;
              int hantei_core = 0;
              std::string source_word = id2node[start_id];
              std::string target_word = id2node[target_id];
              if(core_nodes_dict.find(target_word)!=core_nodes_dict.end() && core_nodes_dict.find(source_word)!=core_nodes_dict.end()){
                  hantei_core = 1;}
              if(hantei_core==1 && start_id != target_id){
                  long long local_target = id2label_prop_id[target_id];
                  label_prop_row.push_back(i);
                  label_prop_col.push_back(local_target);
                  triplet_list.push_back(T(i,local_target,float(1.0)));
                  row_counter += 1;
              }
            }
        }

        // ADDED //
        p_matrix.resize(0,0);
        p_matrix.setZero();
        p_matrix.data().squeeze();
        p_matrix.resize(label_prop_counter,label_prop_counter);
        p_matrix.setFromTriplets(triplet_list.begin(), triplet_list.end());
        for(long long i=0;i<label_prop_counter;++i){
            float total = p_matrix.row(i).sum();
            p_matrix.row(i) = p_matrix.row(i) / total;
            //float total = sparse_core_matrix.col(i).sum();
            //sparse_core_matrix.col(i) = sparse_core_matrix.col(i) / total;
        }
    }

    Eigen::MatrixXf depth_matrix_revised,depth_order_matrix_revised;
    std::unordered_map<int,int> depth_local_column2column,depth_order_local_column2column;

    void TfIdfDepthMatrix(int pattern,const int &max_depth){

        // NEW!
        int num_relation = int(depth_matrix.cols())/max_depth;
        if(pattern == 2){
            depth_matrix_revised.resize(0,0);
            depth_matrix_revised.setZero();
            depth_matrix_revised.setZero(depth_matrix.rows(),depth_matrix.cols());

            std::cout << "Number of Relation: " << num_relation << std::endl;
            for(int i=0;i<depth_matrix_revised.rows();++i){
                for(int ii=0;ii<max_depth;++ii){
                    float value = depth_matrix.block(i,ii*num_relation,1,num_relation).sum();
                    for(int j=(ii)*num_relation;j<(ii+1)*num_relation;++j){
                        if(depth_matrix(i,j)>0){
                            depth_matrix_revised(i,j) = depth_matrix(i,j)/value;
                        }
                    }
                }
            }
            depth_order_matrix_revised.resize(0,0);
            depth_order_matrix_revised.setZero();
            depth_order_matrix_revised.setZero(depth_order_matrix.rows(),depth_order_matrix.cols());
            for(int i=0;i<depth_order_matrix_revised.rows();++i){
                for(int ii=0;ii<max_depth;++ii){
                    float value = depth_order_matrix.block(i,ii*num_relation,1,num_relation).sum();
                    for(int j=(ii)*num_relation;j<(ii+1)*num_relation;++j){
                        if(depth_order_matrix(i,j)>0){
                            depth_order_matrix_revised(i,j) = depth_order_matrix(i,j)/value;
                        }
                    }
                }
            }

        }else{
            depth_local_column2column.clear();
            depth_order_local_column2column.clear();
            // depth_matrix_revised
            int num_columns = 0;
            for(int i=0;i<depth_matrix.cols();++i){
                if(depth_matrix_col(0,i)>0){
                    depth_local_column2column[num_columns] = i;
                    num_columns += 1;}}
            depth_matrix_revised.resize(0,0);
            depth_matrix_revised.setZero();
            depth_matrix_revised.setZero(depth_matrix.rows(),num_columns);
            for(int i=0;i<num_columns;++i){
                int place = depth_local_column2column[i];
                depth_matrix_revised.col(i) = depth_matrix.col(place);}
            float num_rows = float(depth_matrix.rows());
            for(int i=0;i<depth_matrix_revised.rows();++i){
                std::vector<float> row_sum;
                for(int ii=0;ii<max_depth;++ii){
                    float value0 = depth_matrix.block(i,ii*num_relation,1,num_relation).sum();
                    row_sum.push_back(value0);}
                for(int j=0;j<depth_matrix_revised.cols();++j){
                    float value = depth_matrix_revised(i,j);
                    if(value>0){
                        int place = depth_local_column2column[j];
                        if(pattern==1){
                            double moto_place0 = double(depth_local_column2column[i]);
                            int moto_place = int(floor(moto_place0/double(num_relation)));
                            value = value / depth_matrix_row(i,0) * log(num_rows/depth_matrix_col(0,place));
                            //value = value / row_sum[moto_place] * log(num_rows/depth_matrix_col(0,place));
                        }else{
                            value = value / depth_matrix_row(i,0);}
                        depth_matrix_revised(i,j) = value;}}}
            // depth_order_matrix_revised
            num_columns = 0;
            for(int i=0;i<depth_order_matrix.cols();++i){
                if(depth_order_matrix_col(0,i)>0){
                    depth_order_local_column2column[num_columns] = i;
                    num_columns += 1;
                }
            }
            depth_order_matrix_revised.resize(0,0);
            depth_order_matrix_revised.setZero();
            depth_order_matrix_revised.setZero(depth_order_matrix.rows(),num_columns);
            for(int i=0;i<num_columns;++i){
                int place = depth_order_local_column2column[i];
                depth_order_matrix_revised.col(i) = depth_order_matrix.col(place);
            }
            num_rows = float(depth_order_matrix.rows());
            for(int i=0;i<depth_order_matrix_revised.rows();++i){
                std::vector<float> row_sum;
                for(int ii=0;ii<max_depth;++ii){
                    float value0 = depth_order_matrix.block(i,ii*num_relation,1,num_relation).sum();
                    row_sum.push_back(value0);}
                for(int j=0;j<depth_order_matrix_revised.cols();++j){
                    float value = depth_order_matrix_revised(i,j);
                    if(value>0){
                        int place = depth_order_local_column2column[j];
                        if(pattern==1){
                            double moto_place0 = double(depth_local_column2column[i]);
                            int moto_place = int(floor(moto_place0/double(num_relation)));
                            value = value / depth_order_matrix_row(i,0) * log(num_rows/depth_order_matrix_col(0,place));
                            //value = value / row_sum[moto_place] * log(num_rows/depth_order_matrix_col(0,place));
                        }else{
                            value = value / depth_order_matrix_row(i,0);}
                        depth_order_matrix_revised(i,j) = value;}}}}
    }

    std::unordered_map<long long,long long> moto_edge_id2sorted_edge_id;
    void ReorganizePathMatrix(const std::string &output_file,int pattern,Eigen::MatrixXi &moto_vector){
        moto_edge_id2sorted_edge_id.clear();
        for(long long i=0;i<moto_vector.size();++i){
            moto_edge_id2sorted_edge_id[moto_vector(i)] = i;}
        long long line_counter = 0;
        std::ofstream ofs(output_file);
        path_row.clear();path_row.shrink_to_fit();
        path_col.clear();path_col.shrink_to_fit();
        path_value.clear();path_value.shrink_to_fit();
        float num_rows = float(path_matrix_sparse.rows());
        for(int k=0;k<path_matrix_sparse.outerSize();++k){
            for(Eigen::SparseMatrix<float, Eigen::ColMajor,long long>::InnerIterator it(path_matrix_sparse,k);it;++it){
                float value = it.value();
                //path_matrix_sparse.coeffRef(it.row(),it.col()) = value;
                long long newrow = moto_edge_id2sorted_edge_id[it.row()];
                ofs << newrow << "," << it.col() << "," << value << std::endl;
                if(line_counter % 20000000 == 0) std::cout << "Line: " << line_counter << std::endl;
                line_counter += 1;}}
    }

    std::vector<long long> edge_id_gather_1_row,edge_id_gather_1_cnt;
    std::vector<long long> edge_id_gather_2_row,edge_id_gather_2_cnt;
    std::vector<long long> edge_id_gather_3_row,edge_id_gather_3_cnt;
    std::vector<long long> edge_id_path_rnn_1_1_row,edge_id_path_rnn_1_1_col;
    std::vector<long long> edge_id_path_rnn_2_1_row,edge_id_path_rnn_2_1_col;
    std::vector<long long> edge_id_path_rnn_2_2_row,edge_id_path_rnn_2_2_col;
    std::vector<long long> edge_id_path_rnn_3_1_row,edge_id_path_rnn_3_1_col;
    std::vector<long long> edge_id_path_rnn_3_2_row,edge_id_path_rnn_3_2_col;
    std::vector<long long> edge_id_path_rnn_3_3_row,edge_id_path_rnn_3_3_col;
    std::vector<float> edge_id_path_rnn_1_0,edge_id_path_rnn_2_0,edge_id_path_rnn_3_0;
    std::vector<long long> edge_id_gather_1_col_0,edge_id_gather_2_col_0,edge_id_gather_3_col_0;
    Eigen::MatrixXi edge_id_path_rnn_1_1_row_col,edge_id_gather_1_row_cnt;
    Eigen::MatrixXi edge_id_path_rnn_2_1_row_col,edge_id_path_rnn_2_2_row_col,edge_id_gather_2_row_cnt;
    Eigen::MatrixXi edge_id_path_rnn_3_1_row_col,edge_id_path_rnn_3_2_row_col,edge_id_path_rnn_3_3_row_col,edge_id_gather_3_row_cnt;
    Eigen::MatrixXf edge_id_path_rnn_1,edge_id_path_rnn_2,edge_id_path_rnn_3;
    Eigen::MatrixXi edge_id_gather_1_col,edge_id_gather_2_col,edge_id_gather_3_col;

    void ExtractTicker(){

        double counter = 0.0;
        for(auto itr=id2label_prop_id.begin();itr!=id2label_prop_id.end();++itr){
            counter += 1.0;
            if(counter > 2.0){
                break;
            }
        }
        if(counter == 0.0){
            std::cout << "Please Create id2label_prop_id First" << std::endl;
        }else{
            std::ofstream out_stream("/home/rh/Arbeitsraum/Files/KG/All/serial/label_prop_nodes2ticker.csv");
            long long issue_number = relation2id["issue"];
            for(int i=0;i<relation_train.size();++i){
                std::cout << i << std::endl;
                if(relation_train[i]==issue_number){
                    if(id2label_prop_id.find(source_train[i])!=id2label_prop_id.end()){
                        std::vector<std::string> v1 = Split(id2node[target_train[i]],'-');
                        if(v1.size()>1) out_stream << id2node[source_train[i]] << "," << "issue," << v1[0] <<"," << v1[1] << std::endl;
                    }
                }
            }
            /****/
        }
    }

    template<class Archive>
    void serialize(Archive & archive){
        archive(
          files_edgelist,
          core_nodes_dict,
          node_id_counter,relation_id_counter,edge_id_counter,
          collapsed_edge_id_counter,
          //edge_id_2step_counter,collapsed_edge_id_2step_counter,
          source_train,relation_train,target_train,
          //source_2path_train,target_2path_train,
          node2id,id2node,relation2id,id2relation,
          edge_dict_train,pair_indicator_train,
          edgelist_collapsed_multimap_train,
          //edgelist_collapsed_2step_multimap_train,
          source_multimap_train,target_multimap_train,pair_multimap_train,
          relation_edge,pair2collapsed_edge_id,collapsed_edge_id2edge_id,
          edge_parallel,edge_source,edge_target,
          collapsed_edge_id_is_core,
          edge_depth_order_backpath,collapsed_edge_depth_order_backpath,
          edge_depth_path_relation,collapsed_edge_depth_path_relation
        );
    }
    Eigen::MatrixXf DepthOrderWeight(int &depth,int &order){
         return(depth_order_weight[depth][order]);}
    // Omake
    static std::string GetClassName(){
      return "Edges";}
    void SetName(const std::string &input){
        name = input;}
    std::string GetName(){
        return name;}
};// End of class Edges


void ExtractSubNetwork(std::string &core_node_file,std::string &train_test_split_time,std::string &output_file,int &depth,int &cut_unknown){
    Edges edges("PMD");
    std::cout << "Start: " << std::endl;
    edges.SetEdgeListFromFileGlobal(train_test_split_time,cut_unknown);
    std::cout << "Finished: SetEdgeListFromFileGlobal" << std::endl;
    // Load core nodes
    std::string fname = "/home/rh/Arbeitsraum/Files/KG/All/serial/" + core_node_file;
    std::ifstream file(fname);
    std::string str;
    int temp_counter=0;
    while(std::getline(file,str)){
        if(temp_counter > 0){
            std::vector<std::string> elements = Split(str,',');
            if(edges.core_nodes_dict.find(elements[0])==edges.core_nodes_dict.end()){
                edges.core_nodes_dict[elements[0]] = 1;
            }
        }
        temp_counter += 1;
    }
    edges.CreateSubNetwork(depth,output_file);
    std::cout << "Number of core nodes: " << boost::lexical_cast<std::string>(temp_counter) << std::endl;
    std::cout << "Out of Function" << std::endl;
}

void Serialize(std::string &core_node_file,std::string &file_location_text,std::string &filename,int &depth,std::string &train_test_split_time,int &steps){

    Edges edges("PMD");
    std::ifstream input_stream(file_location_text);
    std::string line;
    while(input_stream && std::getline(input_stream,line)){
        std::cout << "Going to read " << line << std::endl;
        edges.files_edgelist.push_back(Trimstring(line));
    }

    // Load core nodes
    std::string fname = "/home/rh/Arbeitsraum/Files/KG/All/serial/" + core_node_file;
    std::ifstream file(fname);
    std::string str;

    int temp_counter = 0;
    edges.core_nodes_dict.clear();
    while(std::getline(file,str)){
        if(temp_counter>0){
            std::vector<std::string> elements = Split(str,',');
            if(edges.core_nodes_dict.find(elements[0])==edges.core_nodes_dict.end()){
                edges.core_nodes_dict[elements[0]] = 1;
                std::cout << elements[0] << std::endl;}}
        temp_counter += 1;
    }

    std::cout << "Start: " << std::endl;
    edges.SetEdgeListFromFile(train_test_split_time);
    std::cout << "Finished: SetEdgeListFromFile" << std::endl;
    std::cout << "edge_id_counter " << edges.edge_id_counter << std::endl;

    // DEPRECATED
    //edges.CreateEdgeid2Edgeid();
    //std::cout << "Finished: CreateEdgeid2Edgeid" << std::endl;

    edges.AllBackPath3(depth,1,steps);
    std::cout << "Finished: AllBackPath" << std::endl;

    std::ofstream output_stream(filename);
    {
        cereal::BinaryOutputArchive o_archive(output_stream);
        o_archive(edges);
    }
    /****/
}

class Edges DeSerialize(std::string &filename){
    Edges edges(filename);;
    std::ifstream input_stream(filename);
    {
        cereal::BinaryInputArchive i_archive(input_stream);
        i_archive(edges);
    }
    return(edges);
}

PYBIND11_PLUGIN(hetinf_pmd){
    // Definition of our module
    py::module m("hetinf_pmd", "Hetero Info Plugin");

    // OMAJINAI: Binders
    py::bind_vector<std::vector<std::string>>(m,"StringVector");
    py::bind_vector<std::vector<int>>(m,"IntVector");
    py::bind_vector<std::vector<float>>(m,"FloatVector");
    py::bind_map<std::unordered_map<std::string,int>>(m,"StringIntMap");
    py::bind_map<std::unordered_map<int,std::string>>(m,"IntStringMap");
    py::bind_map<std::unordered_map<std::string,double>>(m,"StringDoubleMap");
    py::bind_map<std::unordered_map<int,int>>(m,"IntIntMap");
    py::bind_map<std::unordered_multimap<int,int>>(m,"IntIntMultiMap");
    py::bind_vector<std::vector<long long>>(m, "LongVector");
    py::bind_map<std::unordered_map<std::string,long long>>(m,"StringLongMap");
    py::bind_map<std::unordered_map<long long,std::string>>(m,"LongStringMap");
    py::bind_map<std::unordered_map<long long,long long>>(m,"LongLongMap");
    py::bind_map<std::unordered_multimap<long long,long long>>(m,"LongLongMultiMap");
    py::bind_map<std::unordered_map<std::string,std::string>>(m,"StringStringMap");
    /***/

    // Functions
    m.def("Sigmoid",&Sigmoid,"Sigmoid Function");
    m.def("SigmoidFast",&SigmoidFast,"Sigmoid Fast Function");
    m.def("Relu",&Relu,"ReLU Function");
    m.def("ContorPair",&ContorPair,"ContorPair");
    m.def("ContorTuple",&ContorTuple,"ContorTuple");
    m.def("Serialize",&Serialize,"Serialize gzip files");
    m.def("DeSerialize",&DeSerialize,"DeSerialize files");
    m.def("ExtractSubNetwork",&ExtractSubNetwork,"Extract Sub Network");


    // Class Node
    py::class_<Nodes>(m,"Nodes",py::dynamic_attr())
        .def(py::init<const std::string &>())

        // Methods
        .def("ParseHeader",&Nodes::ParseHeader)
        .def("ClearExtractFiles",&Nodes::ClearExtractFiles)
        .def("ParseFile",&Nodes::ParseFile)
        .def("TrimNodeVariables",&Nodes::TrimNodeVariables)


        .def("Melt",&Nodes::Melt)
        .def("GetLongitudeLatitude",&Nodes::GetLongitudeLatitude)

        .def("ClearCoreNodes",&Nodes::ClearCoreNodes)
        .def("CreateLabelDataFrame",&Nodes::CreateLabelDataFrame)
        .def("CreateTickerDataFrame",&Nodes::CreateTickerDataFrame)

        .def_readwrite("header_file",&Nodes::header_file)
        .def_readwrite("header_name",&Nodes::header_name)

        .def_readwrite("extract_files",&Nodes::extract_files)
        .def_readwrite("extract_variables",&Nodes::extract_variables)
        .def_readwrite("node_variables",&Nodes::node_variables)
        .def_readwrite("node_variables2",&Nodes::node_variables2)
        .def_readwrite("node2name",&Nodes::node2name)
        .def_readwrite("node_vector",&Nodes::node_vector)
        .def_readwrite("value_vector",&Nodes::value_vector)
        .def_readwrite("variable_name_vector",&Nodes::variable_name_vector)

        .def_readwrite("node_id",&Nodes::node_id)
        .def_readwrite("longitude",&Nodes::longitude)
        .def_readwrite("latitude",&Nodes::latitude)
        .def_readwrite("node_variable2value",&Nodes::node_variable2value)

        .def_readwrite("core_nodes",&Nodes::core_nodes)
        .def_readwrite("node_vec",&Nodes::node_vec)
        .def_readwrite("name_vec",&Nodes::name_vec)
        .def_readwrite("label1_vec",&Nodes::label1_vec)
        .def_readwrite("label2_vec",&Nodes::label2_vec)
        .def_readwrite("start_vec",&Nodes::start_vec)
        .def_readwrite("ticker_vec",&Nodes::ticker_vec)




        .def_readwrite("node_vector2",&Nodes::node_vector2)
        .def_readwrite("train_positive_time",&Nodes::train_positive_time)
        .def_readwrite("test_positive_time",&Nodes::test_positive_time)
        .def("ClearNodeVector",&Nodes::ClearNodeVector)
        .def("AddCountryIndustry",&Nodes::AddCountryIndustry)



        .def_static("GetClassName",&Nodes::GetClassName)
        .def("SetName",&Nodes::SetName)
        .def("GetName",&Nodes::GetName);

    // Class Edge
    py::class_<Edges>(m,"Edges",py::dynamic_attr())
        .def(py::init<const std::string &>())

        // Methods
        .def("CountLine",&Edges::CountLine)
        .def("GlobalRelation2Cnt",&Edges::GlobalRelation2Cnt)
        .def("SetEdgeListFromFileGlobal",&Edges::SetEdgeListFromFileGlobal)
        .def("SetEdgeListFromFile",&Edges::SetEdgeListFromFile)
        .def("InitializeTopicMatrix",&Edges::InitializeTopicMatrix)
        .def("InitializeWeightMatrix",&Edges::InitializeWeightMatrix)
        .def("InitializeSparseCoreMatrix",&Edges::InitializeSparseCoreMatrix)
        .def("MeanFieldUpdate",&Edges::MeanFieldUpdate)
        .def("MeanFieldUpdateParallel",&Edges::MeanFieldUpdateParallel)
        .def("ExtractTopic",&Edges::ExtractTopic)
        .def("EdgeDepthOrderTopic",&Edges::EdgeDepthOrderTopic)
        .def("EdgeDepthOrderBackPath",&Edges::EdgeDepthOrderBackPath)
        .def("CreateLocalNetwork",&Edges::CreateLocalNetwork)
        .def("DepthOrderWeight",&Edges::DepthOrderWeight)
        .def("DegreeDistribution",&Edges::DegreeDistribution)
        .def("ClearTrainPositiveTime",&Edges::ClearTrainPositiveTime)
        .def("CompareTimeString",&Edges::CompareTimeString)
        .def("LPAll",&Edges::LPAll)
        .def("TrackLabel",&Edges::TrackLabel)
        .def("FindKeyID",&Edges::FindKeyID)
        .def("ShowOneStep",&Edges::ShowOneStep)
        .def("ShowRelation",&Edges::ShowRelation)
        .def("PathFeature",&Edges::PathFeature)

        // NEW!
        .def("CreateObjects",&Edges::CreateObjects)
        .def("CreateObjectsTrim",&Edges::CreateObjectsTrim)
        .def("CreatePath2Count",&Edges::CreatePath2Count)
        .def("CreatePath2Count2",&Edges::CreatePath2Count2)
        .def("CreateDepthMatrix",&Edges::CreateDepthMatrix)
        .def("CreateNodeDegree",&Edges::CreateNodeDegree)
        .def("Specificity",&Edges::Specificity)
        .def("TfIdfDepthMatrix",&Edges::TfIdfDepthMatrix)
        .def("ReorganizePathMatrix",&Edges::ReorganizePathMatrix)
        .def("CreateJumpMatrix",&Edges::CreateJumpMatrix)
        .def("ClearLabelNodes",&Edges::ClearLabelNodes)
        .def("InitializeJumpCoreMatrix",&Edges::InitializeJumpCoreMatrix)
        .def("CreateSpecificity",&Edges::CreateSpecificity)
        .def("RunCommon",&Edges::RunCommon)
        .def("ClearLabels",&Edges::ClearLabels)
        .def("CreateObjectsMult",&Edges::CreateObjectsMult)
        .def("MultLP",&Edges::MultLP)


        .def_readwrite("sparse_all_matrix",&Edges::sparse_all_matrix)
        .def_readwrite("train_positive_time",&Edges::train_positive_time)
        .def_readwrite("global_relation",&Edges::global_relation)
        .def_readwrite("global_relation_cnt",&Edges::global_relation_cnt)
        .def_readwrite("files_edgelist",&Edges::files_edgelist)
        .def_readwrite("core_nodes_dict",&Edges::core_nodes_dict)
        .def_readwrite("node_id_counter",&Edges::node_id_counter)
        .def_readwrite("relation_id_counter",&Edges::relation_id_counter)
        .def_readwrite("edge_id_counter",&Edges::edge_id_counter)
        .def_readwrite("collapsed_edge_id_counter",&Edges::collapsed_edge_id_counter)
        .def_readwrite("source_train",&Edges::source_train)
        .def_readwrite("relation_train",&Edges::relation_train)
        .def_readwrite("target_train",&Edges::target_train)
        .def_readwrite("node2id",&Edges::node2id)
        .def_readwrite("id2node",&Edges::id2node)
        .def_readwrite("relation2id",&Edges::relation2id)
        .def_readwrite("id2relation",&Edges::id2relation)
        .def_readwrite("relation2cnt",&Edges::relation2cnt)
        .def_readwrite("relation",&Edges::relation)
        .def_readwrite("relation_cnt",&Edges::relation_cnt)
        .def_readwrite("core_node_id_counter",&Edges::core_node_id_counter)
        .def_readwrite("core_id2id",&Edges::core_id2id)
        .def_readwrite("id2core_id",&Edges::id2core_id)
        .def_readwrite("edge_dict_train",&Edges::edge_dict_train)
        .def_readwrite("pair_indicator_train",&Edges::pair_indicator_train)
        .def_readwrite("edgelist_collapsed_multimap_train",&Edges::edgelist_collapsed_multimap_train)
        .def_readwrite("source_multimap_train",&Edges::source_multimap_train)
        .def_readwrite("target_multimap_train",&Edges::target_multimap_train)
        .def_readwrite("pair_multimap_train",&Edges::pair_multimap_train)
        .def_readwrite("relation_edge",&Edges::relation_edge)
        .def_readwrite("pair2collapsed_edge_id",&Edges::pair2collapsed_edge_id)
        .def_readwrite("collapsed_edge_id2edge_id",&Edges::collapsed_edge_id2edge_id)
        .def_readwrite("edge_parallel",&Edges::edge_parallel)
        .def_readwrite("edge_source",&Edges::edge_source)
        .def_readwrite("edge_target",&Edges::edge_target)
        .def_readwrite("topic_matrix",&Edges::topic_matrix)
        .def_readwrite("output_weight",&Edges::output_weight)
        .def_readwrite("edge_matrix",&Edges::edge_matrix)
        .def_readwrite("edge_matrix_one_hot",&Edges::edge_matrix_one_hot)
        .def_readwrite("temp_label",&Edges::temp_label)
        .def_readwrite("use_index",&Edges::use_index)
        .def_readwrite("node_scores",&Edges::node_scores)
        .def_readwrite("predict_local2global",&Edges::predict_local2global)
        .def_readwrite("predict_global2local",&Edges::predict_global2local)
        .def_readwrite("feature_matrix",&Edges::feature_matrix)
        .def_readwrite("target_matrix",&Edges::target_matrix)
        .def_readwrite("feature_matrix_test",&Edges::feature_matrix_test)
        .def_readwrite("target_indices_test",&Edges::target_indices_test)
        .def_readwrite("target_matrix_test",&Edges::target_matrix_test)
        .def_readwrite("collapsed_edge_id_is_core",&Edges::collapsed_edge_id_is_core)
        .def_readwrite("node_degree",&Edges::node_degree)
        .def_readwrite("degree",&Edges::degree)
        .def_readwrite("label_prop_id2id",&Edges::label_prop_id2id)
        .def_readwrite("id2label_prop_id",&Edges::id2label_prop_id)
        .def_readwrite("label_prop_row",&Edges::label_prop_row)
        .def_readwrite("label_prop_col",&Edges::label_prop_col)
        .def_readwrite("label_prop_row_col",&Edges::label_prop_row_col)
        .def_readwrite("label_prop_weight",&Edges::label_prop_weight)
        .def_readwrite("label_prop_counter",&Edges::label_prop_counter)
        .def_readwrite("y_init_train",&Edges::y_init_train)
        .def_readwrite("y_init_test",&Edges::y_init_test)
        .def_readwrite("y_full",&Edges::y_full)
        .def_readwrite("eval_indices_train",&Edges::eval_indices_train)
        .def_readwrite("eval_indices_test",&Edges::eval_indices_test)
        .def_readwrite("label_prop_inverse_train",&Edges::label_prop_inverse_train)
        .def_readwrite("label_prop_inverse_test",&Edges::label_prop_inverse_test)
        .def_readwrite("train_number",&Edges::train_number)
        .def_readwrite("devel_number",&Edges::devel_number)
        .def_readwrite("test_number",&Edges::test_number)
        .def_readwrite("core_source",&Edges::core_source)
        .def_readwrite("core_target",&Edges::core_target)
        .def_readwrite("core_time",&Edges::core_time)

        // DEPRECATED
        .def("RecoverEdgeTime",&Edges::RecoverEdgeTime)
        .def_readwrite("time_train",&Edges::time_train)
        .def_readwrite("edge_dict_train_time",&Edges::edge_dict_train_time)
        .def_readwrite("edge_id_counter_temp",&Edges::edge_id_counter_temp)
        .def_readwrite("lp_all_pre",&Edges::lp_all_pre)
        .def_readwrite("lp_all_test",&Edges::lp_all_test)
        .def_readwrite("final_feature",&Edges::final_feature)
        .def_readwrite("final_source",&Edges::final_source)
        .def_readwrite("final_target",&Edges::final_target)
        .def_readwrite("key_id2count",&Edges::key_id2count)
        .def_readwrite("node2count",&Edges::node2count)
        .def_readwrite("path2count",&Edges::path2count)
        .def_readwrite("collapsed_edge_id2num_path",&Edges::collapsed_edge_id2num_path)

        // NEW!
        .def_readwrite("path_matrix",&Edges::path_matrix)
        .def_readwrite("path_matrix_row",&Edges::path_matrix_row)
        .def_readwrite("path_matrix_col",&Edges::path_matrix_col)
        .def_readwrite("depth_matrix",&Edges::depth_matrix)
        .def_readwrite("depth_matrix_row",&Edges::depth_matrix_row)
        .def_readwrite("depth_matrix_col",&Edges::depth_matrix_col)
        .def_readwrite("depth_order_matrix",&Edges::depth_order_matrix)
        .def_readwrite("depth_order_matrix_row",&Edges::depth_order_matrix_row)
        .def_readwrite("depth_order_matrix_col",&Edges::depth_order_matrix_col)
        .def_readwrite("path_matrix_sparse",&Edges::path_matrix_sparse)
        .def_readwrite("depth_matrix_sparse",&Edges::depth_matrix_sparse)
        .def_readwrite("depth_order_matrix_sparse",&Edges::depth_order_matrix_sparse)
        .def_readwrite("train_positive_list",&Edges::train_positive_list)
        .def_readwrite("test_positive_list",&Edges::test_positive_list)
        .def_readwrite("train_positive_time_list",&Edges::train_positive_time_list)
        .def_readwrite("test_positive_time_list",&Edges::test_positive_time_list)
        .def_readwrite("local_relation_id2relation_id",&Edges::local_relation_id2relation_id)
        .def_readwrite("relation_id2local_relation_id",&Edges::relation_id2local_relation_id)
        .def_readwrite("node2degree",&Edges::node2degree)
        .def_readwrite("include_path",&Edges::include_path)
        .def_readwrite("depth_matrix_revised",&Edges::depth_matrix_revised)
        .def_readwrite("depth_order_matrix_revised",&Edges::depth_order_matrix_revised)
        .def_readwrite("depth_local_column2column",&Edges::depth_local_column2column)
        .def_readwrite("depth_order_local_column2column",&Edges::depth_order_local_column2column)
        .def_readwrite("path_row",&Edges::path_row)
        .def_readwrite("path_col",&Edges::path_col)
        .def_readwrite("path_value",&Edges::path_value)
        .def_readwrite("label_nodes",&Edges::label_nodes)
        .def_readwrite("edge_id_gather_1_row",&Edges::edge_id_gather_1_row)
        .def_readwrite("edge_id_gather_1_col",&Edges::edge_id_gather_1_col)
        .def_readwrite("edge_id_gather_2_row",&Edges::edge_id_gather_2_row)
        .def_readwrite("edge_id_gather_2_col",&Edges::edge_id_gather_2_col)
        .def_readwrite("edge_id_gather_3_row",&Edges::edge_id_gather_3_row)
        .def_readwrite("edge_id_gather_3_col",&Edges::edge_id_gather_3_col)
        .def_readwrite("edge_id_path_rnn_1_1_row",&Edges::edge_id_path_rnn_1_1_row)
        .def_readwrite("edge_id_path_rnn_1_1_col",&Edges::edge_id_path_rnn_1_1_col)
        .def_readwrite("edge_id_path_rnn_1",&Edges::edge_id_path_rnn_1)
        .def_readwrite("edge_id_path_rnn_2_1_row",&Edges::edge_id_path_rnn_2_1_row)
        .def_readwrite("edge_id_path_rnn_2_1_col",&Edges::edge_id_path_rnn_2_1_col)
        .def_readwrite("edge_id_path_rnn_2_2_row",&Edges::edge_id_path_rnn_2_2_row)
        .def_readwrite("edge_id_path_rnn_2_2_col",&Edges::edge_id_path_rnn_2_2_col)
        .def_readwrite("edge_id_path_rnn_2",&Edges::edge_id_path_rnn_2)
        .def_readwrite("edge_id_path_rnn_3_1_row",&Edges::edge_id_path_rnn_3_1_row)
        .def_readwrite("edge_id_path_rnn_3_1_col",&Edges::edge_id_path_rnn_3_1_col)
        .def_readwrite("edge_id_path_rnn_3_2_row",&Edges::edge_id_path_rnn_3_2_row)
        .def_readwrite("edge_id_path_rnn_3_2_col",&Edges::edge_id_path_rnn_3_2_col)
        .def_readwrite("edge_id_path_rnn_3_3_row",&Edges::edge_id_path_rnn_3_3_row)
        .def_readwrite("edge_id_path_rnn_3_3_col",&Edges::edge_id_path_rnn_3_3_col)
        .def_readwrite("edge_id_path_rnn_3",&Edges::edge_id_path_rnn_3)
        .def_readwrite("edge_id_gather_1_cnt",&Edges::edge_id_gather_1_cnt)
        .def_readwrite("edge_id_gather_2_cnt",&Edges::edge_id_gather_2_cnt)
        .def_readwrite("edge_id_gather_3_cnt",&Edges::edge_id_gather_3_cnt)
        .def_readwrite("specificity",&Edges::specificity)
        .def_readwrite("edge_id_path_rnn_1_1_row_col",&Edges::edge_id_path_rnn_1_1_row_col)
        .def_readwrite("edge_id_path_rnn_2_1_row_col",&Edges::edge_id_path_rnn_2_1_row_col)
        .def_readwrite("edge_id_path_rnn_2_2_row_col",&Edges::edge_id_path_rnn_2_2_row_col)
        .def_readwrite("edge_id_path_rnn_3_1_row_col",&Edges::edge_id_path_rnn_3_1_row_col)
        .def_readwrite("edge_id_path_rnn_3_2_row_col",&Edges::edge_id_path_rnn_3_2_row_col)
        .def_readwrite("edge_id_path_rnn_3_3_row_col",&Edges::edge_id_path_rnn_3_3_row_col)
        .def_readwrite("edge_id_gather_1_row_cnt",&Edges::edge_id_gather_1_row_cnt)
        .def_readwrite("edge_id_gather_2_row_cnt",&Edges::edge_id_gather_2_row_cnt)
        .def_readwrite("edge_id_gather_3_row_cnt",&Edges::edge_id_gather_3_row_cnt)
        .def_readwrite("edge_id_path_rnn_1_0",&Edges::edge_id_path_rnn_1_0)
        .def_readwrite("edge_id_path_rnn_2_0",&Edges::edge_id_path_rnn_2_0)
        .def_readwrite("edge_id_path_rnn_3_0",&Edges::edge_id_path_rnn_3_0)
        .def_readwrite("edge_id_gather_1_col_0",&Edges::edge_id_gather_1_col_0)
        .def_readwrite("edge_id_gather_2_col_0",&Edges::edge_id_gather_2_col_0)
        .def_readwrite("edge_id_gather_3_col_0",&Edges::edge_id_gather_3_col_0)
        .def_readwrite("use_label_vector",&Edges::use_label_vector)
        .def_readwrite("label_matrix_mult_train",&Edges::label_matrix_mult_train)
        .def_readwrite("label_matrix_mult_test",&Edges::label_matrix_mult_test)
        .def_readwrite("sparse_core_matrix",&Edges::sparse_core_matrix)
        .def_readwrite("update_indices",&Edges::update_indices)

        .def("ExtractTicker",&Edges::ExtractTicker)

        // NEW!
        .def_readwrite("collapsed_path2id",&Edges::collapsed_path2id)
        .def_readwrite("id2collapsed_path",&Edges::id2collapsed_path)
        .def_readwrite("nearby_source",&Edges::nearby_source)
        .def_readwrite("nearby_target",&Edges::nearby_target)
        .def_readwrite("nearby_weight",&Edges::nearby_weight)
        .def_readwrite("local_pair2index",&Edges::local_pair2index)
        .def_readwrite("label_prop_I_train",&Edges::label_prop_I_train)
        .def_readwrite("label_prop_I_test",&Edges::label_prop_I_test)
        .def_readwrite("moto_edge_id2sorted_edge_id",&Edges::moto_edge_id2sorted_edge_id)
        .def_readwrite("path2collapsed_path_id",&Edges::path2collapsed_path_id)


        .def("CreateSparseWeightOneHotBackPath2",&Edges::CreateSparseWeightOneHotBackPath2)
        .def("CreatePathMatrix3",&Edges::CreatePathMatrix3)
        .def("CalculateCommon",&Edges::CalculateCommon)
        .def("CreateLocalPair2Index",&Edges::CreateLocalPair2Index)
        .def("FindNearbyLabel",&Edges::FindNearbyLabel)
        .def("GetResult",&Edges::GetResult)
        .def("CreateMultMatrix",&Edges::CreateMultMatrix)

        .def_static("GetClassName", &Edges::GetClassName)
        .def("SetName", &Edges::SetName)
        .def("GetName", &Edges::GetName);

    return m.ptr();
}



int main(){
    return 0;
}
