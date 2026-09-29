#include <bits/stdc++.h>
#include "phifrac.h"
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
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
	string id = "placeholder";
	string song = "track.mp3";
	string picture = "bg.png";
	string chart = "chart.json";
	chart_metadata_t maimaiCharts[7] = {
		{"Default", "0", "Unknown", 0}, // fallback when there isn't '&'
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
	string type = "N"; // use 'L' 'R' instead of '<' '>' '^'.
};
struct maimai_note_t{
	int type = 0; // 0 - tap & hold, 1 - touch & touch hold, 2 - slide
	phifrac start, end, slideDelta = 1;
	bool isHeadBreak=0, isSegsBreak=0, isFirework=0, isEx=0, isHold=0;
	bool doChangeTapToStar=0, doChangeStarToTap=0, doStarRotate=1, isStarHidden/*?*/=0, isStarInstant/*!*/=0; 
	vector<slide_seg_t> segs;
};

struct maimai_chart_data_t{
	bpmlist_t bpmlist;
	vector<maimai_note_t> notes;
	phifrac endTime;
}maimaiCharts[7];


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
	else if(argu == "first_1")	metadata.maimaiCharts[1].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_2")	metadata.maimaiCharts[2].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_3")	metadata.maimaiCharts[3].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_4")	metadata.maimaiCharts[4].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_5")	metadata.maimaiCharts[5].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first_6")	metadata.maimaiCharts[6].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))); 
	else if(argu == "first")	metadata.maimaiCharts[1].msOffset = (int)(1000.0f * to_float(getFileHeaderValue(chart,i))),
								metadata.maimaiCharts[2].msOffset = metadata.maimaiCharts[1].msOffset,
								metadata.maimaiCharts[3].msOffset = metadata.maimaiCharts[1].msOffset,
								metadata.maimaiCharts[4].msOffset = metadata.maimaiCharts[1].msOffset,
								metadata.maimaiCharts[5].msOffset = metadata.maimaiCharts[1].msOffset,
								metadata.maimaiCharts[6].msOffset = metadata.maimaiCharts[1].msOffset;
	else if(argu == "lv_1")		metadata.maimaiCharts[1].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_2")		metadata.maimaiCharts[2].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_3")		metadata.maimaiCharts[3].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_4")		metadata.maimaiCharts[4].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_5")		metadata.maimaiCharts[5].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv_6")		metadata.maimaiCharts[6].lv = getFileHeaderValue(chart,i); 
	else if(argu == "lv")		metadata.maimaiCharts[1].lv = getFileHeaderValue(chart,i),
								metadata.maimaiCharts[2].lv = metadata.maimaiCharts[1].lv,
								metadata.maimaiCharts[3].lv = metadata.maimaiCharts[1].lv,
								metadata.maimaiCharts[4].lv = metadata.maimaiCharts[1].lv,
								metadata.maimaiCharts[5].lv = metadata.maimaiCharts[1].lv,
								metadata.maimaiCharts[6].lv = metadata.maimaiCharts[1].lv;
	else if(argu == "des_1")	metadata.maimaiCharts[1].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_2")	metadata.maimaiCharts[2].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_3")	metadata.maimaiCharts[3].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_4")	metadata.maimaiCharts[4].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_5")	metadata.maimaiCharts[5].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des_6")	metadata.maimaiCharts[6].charter = getFileHeaderValue(chart,i); 
	else if(argu == "des")		metadata.maimaiCharts[1].charter = getFileHeaderValue(chart,i),
								metadata.maimaiCharts[2].charter = metadata.maimaiCharts[1].charter,
								metadata.maimaiCharts[3].charter = metadata.maimaiCharts[1].charter,
								metadata.maimaiCharts[4].charter = metadata.maimaiCharts[1].charter,
								metadata.maimaiCharts[5].charter = metadata.maimaiCharts[1].charter,
								metadata.maimaiCharts[6].charter = metadata.maimaiCharts[1].charter;
	else if(argu == "inote_1")	metadata.maimaiCharts[1].is_valid = 1, changeDiffCallback(1);
	else if(argu == "inote_2")	metadata.maimaiCharts[2].is_valid = 1, changeDiffCallback(2);
	else if(argu == "inote_3")	metadata.maimaiCharts[3].is_valid = 1, changeDiffCallback(3);
	else if(argu == "inote_4")	metadata.maimaiCharts[4].is_valid = 1, changeDiffCallback(4);
	else if(argu == "inote_5")	metadata.maimaiCharts[5].is_valid = 1, changeDiffCallback(5);
	else if(argu == "inote_6")	metadata.maimaiCharts[6].is_valid = 1, changeDiffCallback(6);
	else getFileHeaderValue(chart,i);
}

bool isFloatDigit(char c){
	return (
		'0' <= c && c <= '9' ||
		c == '.' ||
		c == '-'
	);
}

phifrac NoteLenToBeats(float NoteLen){ // o (full note) = 1 NoteLen, = 4 Beats;  quarter note = 4 NoteLen, = 1 beats
	return 4.0f / NoteLen;
}
phifrac secondsToBeats(float bpm, float seconds){
	return seconds * bpm / 60.0f;
}
float beatsToSeconds(float bpm, float beats){
	return beats * 60.0f / bpm;
}
float beatsToSeconds(float bpm, phifrac beats){
	return (beats.integer + (float)(beats.p) / (float)(beats.q)) * 60 / bpm;
}
float NoteLenToSeconds(float bpm, float NoteLen){
	return beatsToSeconds(bpm, NoteLenToBeats(NoteLen));
}
phifrac changeBpmOfBeats(float oldBpm, float newBpm, float beats){
	return secondsToBeats(newBpm, beatsToSeconds(oldBpm, beats));
}

phifrac changeBpmOfBeats(float oldBpm, float newBpm, phifrac beats){
	return secondsToBeats(newBpm, beatsToSeconds(oldBpm, beats));
}
phifrac changeBpmOfNoteLen(float oldBpm, float newBpm, float NoteLen){
	return changeBpmOfBeats(oldBpm, newBpm, NoteLenToBeats(NoteLen));
}
struct comma_length_t{
	bool isSeconds = 0;
	float seconds = 1.0f;
	phifrac beats = 1;
	phifrac to_beats(float bpm){
		if(isSeconds)
			return secondsToBeats(bpm, seconds);
		else
			return beats;
	}
	void reset(){
		isSeconds = 0;
		seconds = 1.0f;
		beats = 1;
	}
};

void addSeg(int curDiff, int lastNote, int id, string type){
	// set type
	maimaiCharts[curDiff].notes[lastNote].type = 2;
	
	// set last seg
	int lastSeg = maimaiCharts[curDiff].notes[lastNote].segs.size()-1;
	maimaiCharts[curDiff].notes[lastNote].segs[lastSeg].end.alpha = 'K';
	maimaiCharts[curDiff].notes[lastNote].segs[lastSeg].end.id = id;
	maimaiCharts[curDiff].notes[lastNote].segs[lastSeg].type = type;
	
	// set new seg
	slide_seg_t newSeg;
	newSeg.start = maimaiCharts[curDiff].notes[lastNote].segs[lastSeg].end;
	maimaiCharts[curDiff].notes[lastNote].segs.push_back(newSeg);
}

void decodeSimai(const string& chart){
	int curDiff = 0;
	metadata.maimaiCharts[0].is_valid = 1;
	bool isFirstBpm = true;
	float curBpm = 120;
	comma_length_t commaLen;
	phifrac bpmStart, curTime, fTime;
	int lastNote = -1;
	
	for(int i=0;i<chart.length();i++){
			 if(chart[i] == '&'){
			decodeFileHeader(chart,i,[&](int newDiff){
				maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm});
				maimaiCharts[curDiff].endTime = curTime;
				curDiff = newDiff;
				isFirstBpm = true;
				curBpm = 120;
				commaLen.reset();
				bpmStart = 0;
				curTime = 0;
				lastNote = -1;
			});
		}
		else if(chart[i] == '('){
			if(!isFirstBpm)
				maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm});
			else isFirstBpm = 0;
			bpmStart = curTime;
			curBpm = to_float(getValue(chart, i, ')'));
		}
		else if(chart[i] == '{'){
			if(chart[i+1] == '#')
				commaLen.isSeconds = 1, i++, commaLen.seconds = to_float(getValue(chart, i, '}'));
			else
				commaLen.isSeconds = 0, commaLen.beats = phifrac(4.0f / (to_float(getValue(chart, i, '}'))));
		}
		else if('1' <= chart[i] && chart[i] <= '8'){
			slide_seg_t slideSeg;
			slideSeg.start.alpha = 'K';
			slideSeg.start.id = chart[i] - '0';
			maimai_note_t note;
			note.type = 0;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
		}
		else if(chart[i] == 'b' && lastNote != -1){ // break
			if(maimaiCharts[curDiff].notes[lastNote].type != 2){
				maimaiCharts[curDiff].notes[lastNote].isHeadBreak = 1;
			}
			else{
				maimaiCharts[curDiff].notes[lastNote].isSegsBreak = 1;
			}
		}
		else if(chart[i] == 'x'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].isEx = 1;
		}
		else if(chart[i] == 'f'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].isFirework = 1;
		}
		else if(chart[i] == 'h'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].isHold = 1;
		}
		else if(chart[i] == '['){
			if(lastNote == -1) continue;
			string timeLen = getValue(chart, i, ']');
			string format = "";
			for(int j=0;j<timeLen.length();j++){
				if(isFloatDigit(timeLen[j])){
					if(j==0) format += 'd';
					else if(!isFloatDigit(timeLen[j-1])) format += 'd';
				} 
				else format += timeLen[j];
			}
			timeLen += "]"; // END
			int j=-1;
			if(format == "d:d"){
				float NoteLen;
				phifrac times;
				NoteLen = to_float(getValue(timeLen, j, ':')); // beats
				times = to_float(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + NoteLenToBeats(NoteLen) * times;
			}
			else if(format == "d"){
				float NoteLen;
				NoteLen = to_float(getValue(timeLen, j, ']')); // beats
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + NoteLenToBeats(NoteLen);
			}
			else if(format == "#d:d"){
				j++; // skip '#'
				float seconds, times;
				seconds = to_float(getValue(timeLen, j, ':'));
				times = to_float(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, seconds * times);
			}
			else if(format == "#d"){
				j++; // skip '#'
				float seconds;
				seconds = to_float(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, seconds);
			}
			else if(format == "d#d:d"){
				float bpm, NoteLen, times;
				bpm = to_float(getValue(timeLen, j, '#'));
				NoteLen = to_float(getValue(timeLen, j, ':')); 
				times = to_float(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].slideDelta = changeBpmOfBeats(bpm, curBpm, 1);
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + times * changeBpmOfNoteLen(bpm, curBpm, NoteLen);
			}
			else if(format == "d#d"){
				float bpm, seconds;
				bpm = to_float(getValue(timeLen, j, '#'));
				seconds = to_float(getValue(timeLen, j, ']')); 
				maimaiCharts[curDiff].notes[lastNote].slideDelta = changeBpmOfBeats(bpm, curBpm, 1);
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, seconds);
			}
			else if(format == "d##d"){
				float slideDeltaSeconds, totalSeconds;
				
				slideDeltaSeconds = to_float(getValue(timeLen, j, '#'));
				j += 1; // skip '##'
				totalSeconds = to_float(getValue(timeLen, j, ']'));
				
				maimaiCharts[curDiff].notes[lastNote].slideDelta = secondsToBeats(curBpm, slideDeltaSeconds);
				
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, totalSeconds);
			}
		}
		else if(chart[i] == ','){
			fTime = fTime + commaLen.to_beats(curBpm);
			curTime = fTime;
			lastNote = -1;
		}
		else if(chart[i] == '`'){
			curTime = curTime + secondsToBeats(curBpm, 0.001);
			lastNote = -1;
		}
		else if(chart[i] == '/'){
			lastNote = -1;
		}
		else if(chart[i] == '-' || chart[i] == 'v' || chart[i] == 's' || chart[i] == 'z' || chart[i] == 'w' ||
			chart[i] == 'p' && chart[i+1] != 'p' || chart[i] == 'q' && chart[i+1] != 'q'){
			if(lastNote == -1 || i+1 >= chart.length()) continue;
			addSeg(curDiff, lastNote, chart[i+1] - '0', string(chart[i], 1));
			i+=1;
		}
		else if(i+1 < chart.length() && (chart[i] == 'p' && chart[i+1] == 'p' || chart[i] == 'q' && chart[i+1] == 'q')){
			if(lastNote == -1 || i+2 >= chart.length()) continue;
			addSeg(curDiff, lastNote, chart[i+2] - '0', string(chart[i], 2));
			i+=2;
		}
		else if(chart[i] == 'V'){
			if(lastNote == -1 || i+2 >= chart.length()) continue;
			addSeg(curDiff, lastNote, chart[i+1] - '0', "-");
			addSeg(curDiff, lastNote, chart[i+2] - '0', "-");
			i+=2;
		}
		else if(chart[i] == '<' || chart[i] == '>' || chart[i] == '^'){
			if(lastNote == -1 || i+1 >= chart.length()) continue;
			int lastSeg = maimaiCharts[curDiff].notes[lastNote].segs.size()-1;
			int start = maimaiCharts[curDiff].notes[lastNote].segs[lastSeg].start.id;
			int end = chart[i+1] - '0';
			if(chart[i] == '<'){
				addSeg(curDiff, lastNote, end, string(" LLRRRRLL"[start], 1));
			}
			else if(chart[i] == '>'){
				addSeg(curDiff, lastNote, end, string(" RRLLLLRR"[start], 1));
			}
			else if(chart[i] == '^'){
				int L = (start + 8 - end) % 8;
				int R = 8 - L;
				addSeg(curDiff, lastNote, end, string("LR"[int(L > R)], 1));
			}
			i+=1;
		}
		else if(chart[i] == 'A' || chart[i] == 'B' || chart[i] == 'D' || chart[i] == 'E'){
			if(i+1 >= chart.length()) continue;
			if(chart[i+1] == '\n' || chart[i+1] == '\r') continue;
			slide_seg_t slideSeg;
			slideSeg.start.alpha = chart[i];
			slideSeg.start.id = chart[i+1] - '0';
			maimai_note_t note;
			note.type = 1;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
			i++;
		}
		else if(chart[i] == 'a' || (chart[i] == 'b' && lastNote == -1/*isn't break*/) || chart[i] == 'd' || chart[i] == 'e'){
			if(i+1 >= chart.length()) continue;
			slide_seg_t slideSeg;
			slideSeg.start.alpha = chart[i] - 32;
			slideSeg.start.id = chart[i+1] - '0';
			maimai_note_t note;
			note.type = 1;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
			i++;
		}
		else if(chart[i] == 'C' || chart[i] == 'c'){
			slide_seg_t slideSeg;
			slideSeg.start.alpha = 'C';
			slideSeg.start.id = 1;
			maimai_note_t note;
			note.type = 1;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
		}
		else if(chart[i] == '$'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].doChangeTapToStar = 1;
			maimaiCharts[curDiff].notes[lastNote].doStarRotate = (i+1 < chart.length() && chart[i+1] == '$')?(i++,true):false;
		}
		else if(chart[i] == '@'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].doChangeStarToTap = 1;
		}
		else if(chart[i] == '?'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].isStarHidden = 1;
		}
		else if(chart[i] == '!'){
			if(lastNote == -1) continue;
			maimaiCharts[curDiff].notes[lastNote].isStarInstant = 1;
		}
		else if(chart[i] == '*'){
			if(lastNote == -1) continue;
			slide_seg_t slideSeg;
			slideSeg.start.alpha = 'K';
			slideSeg.start.id = maimaiCharts[curDiff].notes[lastNote].segs[0].start.id;
			maimai_note_t note;
			note.type = 0;
			note.start = curTime;
			note.end = curTime;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
		}
	}
	maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm});
	maimaiCharts[curDiff].endTime = curTime;
}

struct phigros_control_point_t{
	float x = 0.0f;
	float value = 1.0f;
	int easing = 1;
	phigros_control_point_t() = default;
	phigros_control_point_t(float aX, float defaultValue):x(aX), value(defaultValue){
	}
};
struct phigros_control_list_t{
	vector<phigros_control_point_t> points;
	phigros_control_list_t(float defaultValue){
		points.push_back(phigros_control_point_t{0.0f, defaultValue});
		points.push_back(phigros_control_point_t{9999999.0f, defaultValue});
	}
};
template <typename Argument, int defaultArgument>
struct phigros_event_t{
	int bezier = 0;
	float bezierPoints[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float easingLeft = 0.0f, easingRight = 1.0f;
	int easingType = 1;
	Argument start = defaultArgument, end = defaultArgument;
	phifrac endTime=2, startTime;
	int linkgroup = 0;
};
struct phigros_event_layer_t{
	vector<phigros_event_t<int, 0> > alphaEvents;
	vector<phigros_event_t<float, 0> > moveXEvents, moveYEvents, rotateEvents;
	vector<phigros_event_t<float, 10> > speedEvents;
};
struct phigros_extended_event_layer_t{
	vector<phigros_event_t<float, 0> > inclineEvents;
};
struct phigros_note_t{
	int above = 1;
	int alpha = 255;
	phifrac startTime, endTime;
	int isFake = 0;
	float positionX = 0.0f;
	float size = 1.0f;
	float speed = 1.0f;
	int type = 1; // 1=T, 2=H, 3=F, 4=D
	float visibleTime = 999999.0f;
	float yOffset = 0.0f;
};
struct phigros_judgeline_t{
	int Group = 0;
	float bpmfactor = 1.0f;
	int father = -1;
	int isCover = 1;
	int numOfNotes(){
		return notes.size();
	}
	int zOrder = 0;
	string Name = "Untitled";
	string Texture = "line.png";
	phigros_control_list_t alphaControl{1.0f},
						   posControl{1.0f},
						   sizeControl{1.0f},
						   skewControl{0.0f},
						   yControl{1.0f};
	vector<phigros_event_layer_t> eventLayers;
	phigros_extended_event_layer_t extended;
	vector<phigros_note_t> notes;
};

struct phigros_chart_data_t{
	int RPEVersion = 140;
	string level = "UK Lv.10"; 
	vector<string> judgeLineGroup{1,"Default"};
	vector<phigros_judgeline_t> judgeLineList;
	string multiLineString = "";
	float multiScale = 1.0f;
}phigrosChart;

// coord: ±675 * ±450

void translate_1(maimai_chart_data_t& crt){ 
	{ // 0
		phigros_judgeline_t pj;
		{
			phigros_event_layer_t pel;
			{
				phigros_event_t<float, 0> pey;
				{
					pey.start = -300.0f;
					pey.end   = -300.0f;
					pey.startTime = 0;
					pey.endTime = crt.endTime;
				}
				pel.moveYEvents.push_back(pey);
			}
			pj.eventLayers.push_back(pel);
		}
		phigrosChart.judgeLineList.push_back(pj);
	}
}

int main(){
	string s,origPath;
	ofstream infoOut("output/info.txt");
	
	cout << "要解析的谱面：";
	getline(cin,origPath);
	
	s = get_file(origPath.c_str());
	decodeSimai(s);
	
	cout << "谱面 ID：";
	getline(cin,metadata.id);
	
	cout << "音乐文件名：";
	getline(cin,metadata.song);
	
	cout << "曲绘文件名：";
	getline(cin,metadata.picture);
	
	cout << "文件解析成功。检测到以下难度的谱面。请输入要转换的谱面的编号：\n";
	for(int i=0; i<=6; i++)
		if(metadata.maimaiCharts[i].is_valid) cout << i << ". " << metadata.maimaiCharts[i].diff << "\n";
	int targetDiff;
	cin >> targetDiff;
	
	infoOut << "#\nName: " << metadata.songName 
			<< "\nPath: " << metadata.id 
			<< "\nSong: " << metadata.song
			<< "\nPicture: " << metadata.picture
			<< "\nChart: " << metadata.chart
			<< "\nLevel: " << metadata.maimaiCharts[targetDiff].diff <<" Lv." << metadata.maimaiCharts[targetDiff].lv
			<< "\nComposer: " << metadata.artist
			<< "\nCharter: "<< metadata.maimaiCharts[targetDiff].charter;
	for(maimai_note_t g : maimaiCharts[targetDiff].notes ){
		cout << "Type: " << g.type << " Start: " << g.start << " End: " << g.end << " Key: " << g.segs[0].start.id << "\n";
	}
}
