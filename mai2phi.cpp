#include <bits/stdc++.h>
#include "phifrac.h"
using namespace std;

void wlog(std::string level,std::string msg){
	std::ofstream _opengl_log("mai2phi.log",std::ios::app);
    std::time_t now = std::time(nullptr);
    char time_buf[20];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    _opengl_log << "[" << time_buf << "] [" <<level<<"] "<<msg<<std::endl;
}
std::string get_file(const char* _path){
	std::string code;
	std::ifstream file;
    file.exceptions(std::ifstream::failbit|std::ifstream::badbit);
	try{
		file.open(_path);
		std::stringstream stream;
		stream<<file.rdbuf();
		file.close();
		code=stream.str();
	}
	catch(std::ifstream::failure e){
        wlog("ERROR","FILE_NOT_SUCCESFULLY_READ");
        exit(-1);
	}
	return code;
}

struct chart_metadata_t{
	string diff;
	string lv;
	string charter;
	int msOffset = 0;
	bool is_valid = 0;
	chart_metadata_t(string aDiff, string aLv, string aCharter, int aMsOffset)
		: diff(aDiff), lv(aLv), charter(aCharter), msOffset(aMsOffset){
		}
};
struct metadata_t{
	string songName = "UK";
	string artist = "UK";
	chart_metadata_t charts[7] = {
		{"Default", "0", "Unknown", 0},
		{"Easy", "1", "Unknown", 0},
		{"Basic", "3", "Unknown", 0},
		{"Advanced", "7", "Unknown", 0},
		{"Expert", "10", "Unknown", 0},
		{"Master", "13", "Unknown", 0},
		{"Re:Master", "14", "Unknown", 0}
	};
}metadata;

struct bpm_t{
	phifrac start, end;
	float bpm;
};
struct bpmlist_t{
	vector<bpm_t> bpms;
};
struct check_area_t{ // K* for key; else for screen
	char alpha = 'K';
	int id = 1;
};
struct slide_seg_t{
	check_area_t start, end;
	string type = "-"; 
};
struct note_t{
	int type = 0; // 0 - tap & hold, 1 - touch & touch hold, 2 - slide
	phifrac start, end;
	bool isHeadBreak=0, isSegsBreak=0, isFlash=0, isEx=0, isHold=0; 
	vector<slide_seg_t> segs;
};

struct chart_data_t{
	bpmlist_t bpmlist;
	vector<note_t> notes;
}charts[7];

string getValue(const string& chart,int& i,char end){
	string value;
	while(
		++i<chart.length() && 
		chart[i] != end &&
		(
			(end == '\n' || end == '\r') ? 
			(chart[i] != '\n' && chart[i] != '\r') :
			true
		)){
		value+=chart[i];
	}
	return value;
}
string getFileHeaderValue(const string& chart,int& i){
	return getValue(chart, i, '\n');
}

float to_float(string s){
	float cur=0,bas=0.1f;
	bool isInt=1;
	int sign = 1;
	for(int i=0;i<s.length();i++){
		if(s[i]=='.') isInt=0;
		else if(s[i]=='-') sign = -sign;
		else if(isInt) cur = cur * 10.0f + (float)(s[i]-48);
		else cur += bas * (float)(s[i]-48), bas*=0.1f;
	}
	return sign * cur;
}

void decodeFileHeader(const string& chart,int& i, function<void(int)> changeDiffCallback){
	string argu;
	while(++i<chart.length()&&chart[i]!='='){
		argu+=chart[i];
	}
	     if(argu == "title") 	metadata.songName = getFileHeaderValue(chart,i);
	else if(argu == "artist")	metadata.artist = getFileHeaderValue(chart,i);
	else if(argu == "first_1")	metadata.charts[1].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_2")	metadata.charts[2].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_3")	metadata.charts[3].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_4")	metadata.charts[4].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_5")	metadata.charts[5].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_6")	metadata.charts[6].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first")	metadata.charts[1].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))),
								metadata.charts[2].msOffset = metadata.charts[1].msOffset,
								metadata.charts[3].msOffset = metadata.charts[1].msOffset,
								metadata.charts[4].msOffset = metadata.charts[1].msOffset,
								metadata.charts[5].msOffset = metadata.charts[1].msOffset,
								metadata.charts[6].msOffset = metadata.charts[1].msOffset;
	else if(argu == "lv_1")		metadata.charts[1].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_2")		metadata.charts[2].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_3")		metadata.charts[3].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_4")		metadata.charts[4].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_5")		metadata.charts[5].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_6")		metadata.charts[6].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv")		metadata.charts[1].lv = getFileHeaderValue(chart,i),
								metadata.charts[2].lv = metadata.charts[1].lv,
								metadata.charts[3].lv = metadata.charts[1].lv,
								metadata.charts[4].lv = metadata.charts[1].lv,
								metadata.charts[5].lv = metadata.charts[1].lv,
								metadata.charts[6].lv = metadata.charts[1].lv;
	else if(argu == "des_1")	metadata.charts[1].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_2")	metadata.charts[2].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_3")	metadata.charts[3].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_4")	metadata.charts[4].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_5")	metadata.charts[5].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_6")	metadata.charts[6].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des")		metadata.charts[1].charter = getFileHeaderValue(chart,i),
								metadata.charts[2].charter = metadata.charts[1].charter,
								metadata.charts[3].charter = metadata.charts[1].charter,
								metadata.charts[4].charter = metadata.charts[1].charter,
								metadata.charts[5].charter = metadata.charts[1].charter,
								metadata.charts[6].charter = metadata.charts[1].charter;
	else if(argu == "inote_1")	metadata.charts[1].is_valid = 1, changeDiffCallback(1);
	else if(argu == "inote_2")	metadata.charts[2].is_valid = 1, changeDiffCallback(2);
	else if(argu == "inote_3")	metadata.charts[3].is_valid = 1, changeDiffCallback(3);
	else if(argu == "inote_4")	metadata.charts[4].is_valid = 1, changeDiffCallback(4);
	else if(argu == "inote_5")	metadata.charts[5].is_valid = 1, changeDiffCallback(5);
	else if(argu == "inote_6")	metadata.charts[6].is_valid = 1, changeDiffCallback(6);
	else getFileHeaderValue(chart,i);
}

void decodeSimai(const string& chart){
	int curDiff = 0;
	metadata.charts[0].is_valid = 1;
	bool isFirstBpm = true;
	float curBpm = 120;
	phifrac commaLen(4); // i.e. quarter
	phifrac bpmStart, curTime;
	int lastNote = -1;
	
	for(int i=0;i<chart.length();i++){
			 if(chart[i] == '&'){
			decodeFileHeader(chart,i,[&](int newDiff){
				curDiff = newDiff;
				isFirstBpm = true;
				curBpm = 120;
				commaLen = 4;
				bpmStart = 0;
				curTime = 0;
				lastNote = -1;
			});
		}
		else if(chart[i] == '('){
			if(!isFirstBpm)
				charts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm});
			else isFirstBpm = 0;
			bpmStart = curTime;
			curBpm = to_float(getValue(chart, i, ')'));
		}
		else if(chart[i] == '{'){
			if(chart[i+1] == '#')
				i++,
				commaLen = phifrac(
					to_float(getValue(chart, i, '}')) / 60.0f * curBpm
				);
			else
				commaLen = phifrac(int(to_float(getValue(chart, i, '}'))));
		}
		else if('1' <= chart[i] && chart[i] <= '8'){
			slide_seg_t slideSeg;
			slideSeg.start.alpha = 'K';
			slideSeg.start.id = chart[i] - '0';
			note_t note;
			note.type = 0;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			charts[curDiff].notes.push_back(note);
			lastNote = charts[curDiff].notes.size() - 1;
		}
		else if(chart[i] == 'b'){
			if(charts[curDiff].notes[lastNote].type != 2){
				charts[curDiff].notes[lastNote].isHeadBreak = 1;
			}
			else{
				charts[curDiff].notes[lastNote].isSegsBreak = 1;
			}
		}
		else if(chart[i] == 'x'){
			charts[curDiff].notes[lastNote].isEx = 1;
		}
		else if(chart[i] == 'h'){
			charts[curDiff].notes[lastNote].isHold = 1;
		}
		else if(chart[i] == '['){
			;
		}
		else if(chart[i] == ','){
			;
		}
	}
	charts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm});
}

int main(){
	string s,origPath;
	ofstream infoOut("output/info.txt");
	
	cout << "要解析的谱面：";
	cin >> origPath;
	
	s = get_file(origPath.c_str());
	decodeSimai(s);
	
	cout << "文件解析成功。检测到以下难度的谱面。请输入要转换的谱面的编号：\n";
	for(int i=0; i<=6; i++)
		if(metadata.charts[i].is_valid) cout << i << ". " << metadata.charts[i].diff << "\n";
	int targetDiff;
	cin >> targetDiff;
	
	infoOut << "#\nName: " << metadata.songName 
			<< "\nPath: \nSong: \nPicture: \nChart: \nLevel: " 
				<< metadata.charts[targetDiff].diff <<" Lv." << metadata.charts[targetDiff].lv
			<< "\nComposer: " << metadata.artist
			<< "\nCharter: "<< metadata.charts[targetDiff].charter;
	
	for(auto g:charts[targetDiff].bpmlist.bpms){
		cout << g.bpm << "\n";
	}
}
