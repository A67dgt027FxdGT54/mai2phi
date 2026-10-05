#include <bits/stdc++.h>
#include "phifrac.h"
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;

std::string Tab(int w){
	return string(w, ' ');
}
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
	void outputPhiJSON(ofstream& ofs, int tab, int diff){
		ofs << Tab(tab) << "\"META\" : {\n"
		<< Tab(tab+3) << "\"RPEVersion\" : 140,\n" 
		<< Tab(tab+3) << "\"background\" : \"" << this->picture << "\",\n" 
		<< Tab(tab+3) << "\"composer\" : \"" << this->artist << "\",\n"
		<< Tab(tab+3) << "\"charter\" : \"" << this->maimaiCharts[diff].charter << "\",\n"
		<< Tab(tab+3) << "\"id\" : \"" << this->id << "\",\n"
		<< Tab(tab+3) << "\"level\" : \"" << this->maimaiCharts[diff].diff << " Lv." << this->maimaiCharts[diff].lv << "\",\n"
		<< Tab(tab+3) << "\"name\" : \"" << this->songName << "\",\n"
		<< Tab(tab+3) << "\"offset\" : " << this->maimaiCharts[diff].msOffset << ",\n"
		<< Tab(tab+3) << "\"song\" : \"" << this->song << "\"\n"
		<< Tab(tab) << "},\n";
	}
}metadata;

struct bpm_t{
	phifrac start, end;
	float bpm;
	void outputPhiJSON(ofstream& ofs, int tab, string end){
		ofs << Tab(tab) << "{\n"
		<< Tab(tab+3) << "\"bpm\" : " << fixed << setprecision(2) << bpm << ",\n"
		<< Tab(tab+3) << "\"startTime\" : "; start.outputPhiJSON(ofs); ofs << "\n" 
		<< Tab(tab) << "}" << end << "\n";
	}
};
struct bpmlist_t{
	vector<bpm_t> bpms;
	void outputPhiJSON(ofstream& ofs, int tab){
		ofs << Tab(tab) << "\"BPMList\" : [\n";
		for(int i=0;i<this->bpms.size();i++){
			bpms[i].outputPhiJSON(ofs, tab+3, (i==this->bpms.size()-1?"":","));
		}
		ofs << Tab(tab) << "],\n";
	}
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
phifrac to_phifrac(string s){
	phifrac cur(0), bas(1, 10);
	bool isInt=1;
	phifrac sign = 1;
	for(int i=0;i<s.length();i++){
		if(s[i]=='.') isInt=0;
		else if(s[i]=='-') sign = -sign;
		else if(isInt) cur = cur * 10 + phifrac(s[i]-48);
		else cur += bas * (phifrac)(s[i]-48), bas = bas * phifrac(1, 10);
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
								metadata.maimaiCharts[6].msOffset = metadata.maimaiCharts[1].msOffset,
								metadata.maimaiCharts[0].msOffset = metadata.maimaiCharts[1].msOffset;
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
								metadata.maimaiCharts[6].lv = metadata.maimaiCharts[1].lv,
								metadata.maimaiCharts[0].lv = metadata.maimaiCharts[1].lv;
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
								metadata.maimaiCharts[6].charter = metadata.maimaiCharts[1].charter,
								metadata.maimaiCharts[0].charter = metadata.maimaiCharts[1].charter;
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

phifrac NoteLenToBeats(phifrac NoteLen){ // o (full note) = 1 NoteLen, = 4 Beats;  quarter note = 4 NoteLen, = 1 beats
	return 4 / NoteLen;
}
phifrac secondsToBeats(phifrac bpm, phifrac seconds){
	return seconds * bpm / 60;
}
phifrac beatsToSeconds(phifrac bpm, phifrac beats){
	return beats * 60 / bpm;
}
phifrac NoteLenToSeconds(phifrac bpm, phifrac NoteLen){
	return beatsToSeconds(bpm, NoteLenToBeats(NoteLen));
}
phifrac changeBpmOfBeats(phifrac oldBpm, phifrac newBpm, phifrac beats){
	return secondsToBeats(newBpm, beatsToSeconds(oldBpm, beats));
}
phifrac changeBpmOfNoteLen(phifrac oldBpm, phifrac newBpm, phifrac NoteLen){
	return changeBpmOfBeats(oldBpm, newBpm, NoteLenToBeats(NoteLen));
}
struct comma_length_t{
	bool isSeconds = 0;
	phifrac seconds = 1;
	phifrac beats = 1;
	phifrac to_beats(phifrac bpm){
		if(isSeconds)
			return secondsToBeats(bpm, seconds);
		else
			return beats;
	}
	void reset(){
		isSeconds = 0;
		seconds = 1;
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
	phifrac curBpm = 120;
	comma_length_t commaLen;
	phifrac bpmStart, curTime, fTime;
	int lastNote = -1;
	
	for(int i=0;i<chart.length();i++){
			 if(chart[i] == '&'){
			decodeFileHeader(chart,i,[&](int newDiff){
				maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm.integer + 1.0f * curBpm.p / (1.0f * curBpm.q)});
				maimaiCharts[curDiff].endTime = curTime;
				curDiff = newDiff;
				isFirstBpm = true;
				curBpm = 120;
				commaLen.reset();
				bpmStart = 0;
				curTime = 0;
				lastNote = -1;
				fTime = 0; 
			});
		}
		else if(chart[i] == '('){
			if(!isFirstBpm)
				maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm.integer + 1.0f * curBpm.p / (1.0f * curBpm.q)});
			else isFirstBpm = 0;
			bpmStart = curTime;
			curBpm = to_phifrac(getValue(chart, i, ')'));
		}
		else if(chart[i] == '{'){
			if(chart[i+1] == '#')
				commaLen.isSeconds = 1, i++, commaLen.seconds = to_phifrac(getValue(chart, i, '}'));
			else
				commaLen.isSeconds = 0, commaLen.beats = NoteLenToBeats(to_phifrac(getValue(chart, i, '}')));
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
				phifrac NoteLen;
				phifrac times;
				NoteLen = to_phifrac(getValue(timeLen, j, ':')); 
				times = to_phifrac(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + NoteLenToBeats(NoteLen) * times + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "d"){
				phifrac NoteLen;
				NoteLen = to_phifrac(getValue(timeLen, j, ']')); 
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + NoteLenToBeats(NoteLen) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "#d:d"){
				j++; // skip '#'
				phifrac seconds, times;
				seconds = to_phifrac(getValue(timeLen, j, ':'));
				times = to_phifrac(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, seconds * times) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "#d"){
				j++; // skip '#'
				phifrac seconds;
				seconds = to_phifrac(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].end = maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, seconds) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "d#d:d"){
				phifrac bpm, NoteLen, times;
				bpm = to_phifrac(getValue(timeLen, j, '#'));
				NoteLen = to_phifrac(getValue(timeLen, j, ':')); 
				times = to_phifrac(getValue(timeLen, j, ']'));
				maimaiCharts[curDiff].notes[lastNote].slideDelta = changeBpmOfBeats(bpm, curBpm, 1);
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + times * changeBpmOfNoteLen(bpm, curBpm, NoteLen) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "d#d"){
				phifrac bpm, slidingSeconds;
				bpm = to_phifrac(getValue(timeLen, j, '#'));
				slidingSeconds = to_phifrac(getValue(timeLen, j, ']')); 
				maimaiCharts[curDiff].notes[lastNote].slideDelta = changeBpmOfBeats(bpm, curBpm, 1);
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, slidingSeconds) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
			else if(format == "d##d"){
				phifrac slideDeltaSeconds, slidingSeconds;
				
				slideDeltaSeconds = to_phifrac(getValue(timeLen, j, '#'));
				j += 1; // skip '##'
				slidingSeconds = to_phifrac(getValue(timeLen, j, ']'));
				
				maimaiCharts[curDiff].notes[lastNote].slideDelta = secondsToBeats(curBpm, slideDeltaSeconds);
				
				maimaiCharts[curDiff].notes[lastNote].end = 
					maimaiCharts[curDiff].notes[lastNote].start + secondsToBeats(curBpm, slidingSeconds) + 
					(maimaiCharts[curDiff].notes[lastNote].type == 2 ? maimaiCharts[curDiff].notes[lastNote].slideDelta : 0);
			}
		}
		else if(chart[i] == ','){
			fTime = fTime + commaLen.to_beats(curBpm);
			curTime = fTime;
			lastNote = -1;
		}
		else if(chart[i] == '`'){
			curTime = curTime + secondsToBeats(curBpm, phifrac(1, 1000));
			lastNote = -1;
		}
		else if(chart[i] == '/'){
			lastNote = -1;
		}
		else if(chart[i] == '-' || chart[i] == 'v' || chart[i] == 's' || chart[i] == 'z' || chart[i] == 'w' ||
			chart[i] == 'p' && chart[i+1] != 'p' || chart[i] == 'q' && chart[i+1] != 'q'){
			if(lastNote == -1 || i+1 >= chart.length()) continue;
			addSeg(curDiff, lastNote, chart[i+1] - '0', string(1, chart[i]));
			i+=1;
		}
		else if(i+1 < chart.length() && (chart[i] == 'p' && chart[i+1] == 'p' || chart[i] == 'q' && chart[i+1] == 'q')){
			if(lastNote == -1 || i+2 >= chart.length()) continue;
			addSeg(curDiff, lastNote, chart[i+2] - '0', string(1, chart[i]));
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
				addSeg(curDiff, lastNote, end, string(1, " LLRRRRLL"[start]));
			}
			else if(chart[i] == '>'){
				addSeg(curDiff, lastNote, end, string(1, " RRLLLLRR"[start]));
			}
			else if(chart[i] == '^'){
				int L = (start + 8 - end) % 8;
				int R = 8 - L;
				addSeg(curDiff, lastNote, end, string(1, "LR"[int(L > R)]));
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
			note.start = maimaiCharts[curDiff].notes[lastNote].start;
			note.end = maimaiCharts[curDiff].notes[lastNote].start;
			note.segs.push_back(slideSeg);
			maimaiCharts[curDiff].notes.push_back(note);
			lastNote = maimaiCharts[curDiff].notes.size() - 1;
		}
	}
	maimaiCharts[curDiff].bpmlist.bpms.push_back({bpmStart, curTime, curBpm.integer + 1.0f * curBpm.p / (1.0f * curBpm.q)});
	maimaiCharts[curDiff].endTime = curTime;
}

struct phigros_control_point_t{
	float x = 0.0f;
	float value = 1.0f;
	int easing = 1;
	phigros_control_point_t() = default;
	phigros_control_point_t(float aX, float defaultValue):x(aX), value(defaultValue){
	}
	void outputPhiJSON(ofstream& ofs, int tab, string end, string pointArgType, int argOrder){
		ofs << Tab(tab) << "{\n";
		if(argOrder == 0) 
			ofs << Tab(tab+3) << "\"" << pointArgType << "\" : " << fixed << setprecision(2) << value << ",\n";
		ofs << Tab(tab+3) << "\"easing\" : " << easing << ",\n";
		if(argOrder == 1) 
			ofs << Tab(tab+3) << "\"" << pointArgType << "\" : " << fixed << setprecision(2) << value << ",\n";
		ofs << Tab(tab+3) << "\"x\" : " << fixed << setprecision(2) << x << (argOrder == 2 ? ",\n" : "\n");
		if(argOrder == 2) 
			ofs << Tab(tab+3) << "\"" << pointArgType << "\" : " << fixed << setprecision(2) << value << "\n";
		ofs << Tab(tab) << "}" << end << "\n";
	}
};
struct phigros_control_list_t{
	vector<phigros_control_point_t> points;
	phigros_control_list_t(float defaultValue){
		points.push_back(phigros_control_point_t{0.0f, defaultValue});
		points.push_back(phigros_control_point_t{9999999.0f, defaultValue});
	}
	void outputPhiJSON(ofstream& ofs, int tab, string end, string controlType, string pointArgType, int argOrder){
		ofs << Tab(tab) << "\"" << controlType << "\" : [\n";
		for(int i = 0; i < this->points.size(); i++){
			points[i].outputPhiJSON(ofs, tab+3, i==this->points.size()-1?"":",", pointArgType, argOrder); 
		}
		ofs << Tab(tab) << "]" << end << "\n";
	}
};
template <typename Argument, int defaultArgument, int IsFloat>
struct phigros_event_t{
	int bezier = 0;
	float bezierPoints[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float easingLeft = 0.0f, easingRight = 1.0f;
	int easingType = 1;
	Argument start = defaultArgument, end = Argument(defaultArgument);
	phifrac endTime=2, startTime;
	int linkgroup = 0;
	bool isFloat = IsFloat;
	void outputPhiJSON(ofstream& ofs, int tab, string _end){
		ofs << Tab(tab) << "{\n"
		<< Tab(tab+3) << "\"bezier\" : " << this->bezier << ",\n"
		<< Tab(tab+3) << "\"bezierPoints\" : [ "
			<< fixed << setprecision(2) << this->bezierPoints[0] << ", " 
			<< fixed << setprecision(2) << this->bezierPoints[1] << ", " 
			<< fixed << setprecision(2) << this->bezierPoints[2] << ", " 
			<< fixed << setprecision(2) << this->bezierPoints[3] << " ],\n"
		<< Tab(tab+3) << "\"easingLeft\" : " << fixed << setprecision(2) << this->easingLeft << ",\n"
		<< Tab(tab+3) << "\"easingRight\" : " << fixed << setprecision(2) << this->easingRight << ",\n"
		<< Tab(tab+3) << "\"easingType\" : " << this->easingType << ",\n"
		<< Tab(tab+3) << "\"end\" : ";
			if(isFloat) ofs << fixed << setprecision(2) << this->end;
			else ofs << this->end;
			ofs << ",\n"
		<< Tab(tab+3) << "\"endTime\" : "; this->endTime.outputPhiJSON(ofs); ofs << ",\n"
		<< Tab(tab+3) << "\"linkgroup\" : " << this->linkgroup << ",\n"
		<< Tab(tab+3) << "\"start\" : ";
			if(isFloat) ofs << fixed << setprecision(2) << this->start;
			else ofs << this->start;
			ofs << ",\n"
		<< Tab(tab+3) << "\"startTime\" : "; this->startTime.outputPhiJSON(ofs); ofs << "\n"
		<< Tab(tab) << "}" << _end << "\n";
	}
};
struct phigros_event_layer_t{
	vector<phigros_event_t<int, 0, 0> > alphaEvents;
	vector<phigros_event_t<float, 0, 1> > moveXEvents, moveYEvents, rotateEvents;
	vector<phigros_event_t<float, 10, 1> > speedEvents;
	void outputPhiJSON(ofstream& ofs, int tab, string end){
		ofs << Tab(tab) << "{\n";
		
		ofs << Tab(tab+3) << "\"alphaEvents\" : [\n";
		for(int i=0; i<this->alphaEvents.size();i++){
			this->alphaEvents[i].outputPhiJSON(ofs, tab+6, i==this->alphaEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "],\n";
		
		ofs << Tab(tab+3) << "\"moveXEvents\" : [\n";
		for(int i=0; i<this->moveXEvents.size();i++){
			this->moveXEvents[i].outputPhiJSON(ofs, tab+6, i==this->moveXEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "],\n";
		
		ofs << Tab(tab+3) << "\"moveYEvents\" : [\n";
		for(int i=0; i<this->moveYEvents.size();i++){
			this->moveYEvents[i].outputPhiJSON(ofs, tab+6, i==this->moveYEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "],\n";
		
		ofs << Tab(tab+3) << "\"rotateEvents\" : [\n";
		for(int i=0; i<this->rotateEvents.size();i++){
			this->rotateEvents[i].outputPhiJSON(ofs, tab+6, i==this->rotateEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "],\n";
		
		ofs << Tab(tab+3) << "\"speedEvents\" : [\n";
		for(int i=0; i<this->speedEvents.size();i++){
			this->speedEvents[i].outputPhiJSON(ofs, tab+6, i==this->speedEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "]\n";
		
		ofs << Tab(tab) << "}" << end << "\n";
	}
};
struct phigros_extended_event_layer_t{
	vector<phigros_event_t<float, 0, 1> > inclineEvents;
	void outputPhiJSON(ofstream& ofs, int tab, string end){
		ofs << Tab(tab) << "\"extended\" : {\n";
		
		ofs << Tab(tab+3) << "\"inclineEvents\" : [\n";
		for(int i=0; i<this->inclineEvents.size();i++){
			this->inclineEvents[i].outputPhiJSON(ofs, tab+6, i==this->inclineEvents.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "]\n";
		
		ofs << Tab(tab) << "}" << end << "\n";
	}
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
	void outputPhiJSON(ofstream& ofs, int tab, string end){
		ofs << Tab(tab) << "{\n"
		<< Tab(tab+3) << "\"above\" : " << this->above << ",\n"
		<< Tab(tab+3) << "\"alpha\" : " << this->alpha << ",\n"
		<< Tab(tab+3) << "\"endTime\" : "; this->endTime.outputPhiJSON(ofs); ofs << ",\n"
		<< Tab(tab+3) << "\"isFake\" : " << this->isFake << ",\n"
		<< Tab(tab+3) << "\"positionX\" : " << this->positionX << ",\n"
		<< Tab(tab+3) << "\"size\" : " << fixed << setprecision(2) << this->size << ",\n"
		<< Tab(tab+3) << "\"speed\" : " << fixed << setprecision(2) << this->speed << ",\n"
		<< Tab(tab+3) << "\"startTime\" : "; this->startTime.outputPhiJSON(ofs); ofs << ",\n"
		<< Tab(tab+3) << "\"type\" : " << this->type << ",\n"
		<< Tab(tab+3) << "\"visibleTime\" : " << fixed << setprecision(2) << this->visibleTime << ",\n"
		<< Tab(tab+3) << "\"yOffset\" : " << fixed << setprecision(2) << this->yOffset << "\n"
		<< Tab(tab) << "}" << end << "\n";
	}
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
	void outputPhiJSON(ofstream& ofs, int tab, string end){
		ofs << Tab(tab) << "{\n"
		<< Tab(tab+3) << "\"Group\" : " << this->Group << ",\n" 
		<< Tab(tab+3) << "\"Name\" : \"" << this->Name << "\",\n"
		<< Tab(tab+3) << "\"Texture\" : \"" << this->Texture << "\",\n";
		this->alphaControl.outputPhiJSON(ofs, tab+3, ",", "alphaControl", "alpha", 0);
		ofs << Tab(tab+3) << "\"bpmfactor\" : " << fixed << setprecision(2) << this->bpmfactor << ",\n";
		ofs << Tab(tab+3) << "\"eventLayers\" : [\n";
		for(int i=0;i<this->eventLayers.size();i++){
			this->eventLayers[i].outputPhiJSON(ofs, tab+6, i==this->eventLayers.size()-1?"":",");
		}
		ofs << Tab(tab+3) << "],\n";
		this->extended.outputPhiJSON(ofs, tab+3, ",");
		ofs << Tab(tab+3) << "\"father\" : " << this->father << ",\n"
		<< Tab(tab+3) << "\"isCover\" : " << this->isCover << ",\n";
		
		sort(
			this->notes.begin(), 
			this->notes.end(), 
			[](const phigros_note_t& noteA, const phigros_note_t& noteB)->bool{
				return noteA.startTime < noteB.startTime;
			}
		);
		ofs << Tab(tab+3) << "\"notes\" : [\n";
		for(int i = 0; i < this->notes.size(); i++){
			this->notes[i].outputPhiJSON(ofs, tab+6, i==this->notes.size()-1?"":",");
		} 
		ofs << Tab(tab+3) << "],\n";
		
		ofs << Tab(tab+3) << "\"numOfNotes\" : " << this->numOfNotes() << ",\n";
		this->posControl.outputPhiJSON(ofs, tab+3, ",", "posControl", "pos", 1);
		this->sizeControl.outputPhiJSON(ofs, tab+3, ",", "sizeControl", "size", 1);
		this->skewControl.outputPhiJSON(ofs, tab+3, ",", "skewControl", "skew", 1);
		this->yControl.outputPhiJSON(ofs, tab+3, ",", "yControl", "y", 2);
		ofs << Tab(tab+3) << "\"zOrder\" : " << this->zOrder << "\n";
		ofs << Tab(tab) << "}" << end << "\n";
	}
};

struct phigros_chart_data_t{
	string level = "UK Lv.10"; 
	vector<string> judgeLineGroup{1,"Default"};
	vector<phigros_judgeline_t> judgeLineList;
	string multiLineString = "";
	float multiScale = 1.0f;
	void outputPhiJSON(ofstream& ofs, int tab){
		ofs << Tab(tab) << "\"judgeLineGroup\" : [ ";
		for(int i=0;i<this->judgeLineGroup.size();i++){
			ofs << "\"" << this->judgeLineGroup[i] << "\"" << (i==this->judgeLineGroup.size()-1?" ":", ");
		}
		ofs << "],\n"
		<< Tab(tab) << "\"judgeLineList\" : [\n";
		for(int i = 0; i < this->judgeLineList.size(); i++){
			this->judgeLineList[i].outputPhiJSON(ofs, tab+3, i==this->judgeLineList.size()-1?"":",");
		}
		ofs << Tab(tab) << "],\n"
		<< Tab(tab) << "\"multiLineString\" : \"" << this->multiLineString << "\",\n"
		<< Tab(tab) << "\"multiScale\" : " << fixed << setprecision(2) << this->multiScale << "\n"; 
	}
}phigrosChart;

// coord: ±675 * ±450

float keyIdToPositionX(float keyId){
	return -675.0f + 150.0f * keyId;
}

struct slidePQPPQQSegCache_t{
	float radSAngle=0.0f, radEAngle=0.0f, radAngle=0.0f, sLen=0.0f, eLen=0.0f, cLen=0.0f, totLen=0.0f;
	glm::vec2 O = glm::vec2{0.0f};
	glm::vec2 S = glm::vec2{0.0f};
	glm::vec2 E = glm::vec2{0.0f};
};
float getSlidePQPPQQSegLength(char type, glm::vec2 O, glm::vec2 S, glm::vec2 E, float R, slidePQPPQQSegCache_t& cache){
	float radSGamma = glm::radians(360.0f) - glm::acos(glm::normalize(S - O).x);
	float sLen = glm::length(S - O);
	float sTan = sqrt(sLen * sLen - R * R);
	float radSAlpha = glm::asin(sTan / sLen);
	float radSAngle;
	if(type == 'p') radSAngle = radSGamma + radSAlpha;
	else if(type == 'q') radSAngle = radSGamma - radSAlpha;
	
	float radEGamma = glm::radians(360.0f) - glm::acos(glm::normalize(E - O).x);
	float eLen = glm::length(E - O);
	float eTan = sqrt(eLen * eLen - R * R);
	float radEAlpha = glm::asin(eTan / eLen);
	float radEAngle;
	if(type == 'q') radEAngle = radEGamma + radEAlpha;
	else if(type == 'p') radEAngle = radEGamma - radEAlpha;
	
	float radAngle;
	if(type == 'p') radAngle = fmod(radEAngle - radSAngle + glm::radians(360.0f), glm::radians(360.0f));
	else if(type == 'q') radAngle = fmod(radSAngle - radEAngle + glm::radians(360.0f), glm::radians(360.0f));
	if(radAngle > glm::radians(360.0f) - 1e-4f) radAngle = glm::radians(360.0f);
	if(radAngle < 1e-4f) radAngle = 0;
	cache.radSAngle = radSAngle;
	cache.radEAngle = radEAngle;
	cache.radAngle = radAngle;
	cache.O = O;
	cache.S = S;
	cache.E = E;
	cache.sLen = sTan;
	cache.eLen = eTan;
	cache.cLen = R * radAngle;
	cache.totLen = sTan + R * radAngle + eTan;
	return cache.totLen;
}
float getSlideSegLength(slide_seg_t& ss, slidePQPPQQSegCache_t& cache){
	const float pi = glm::radians(180.0f);
	const glm::vec2 C = glm::vec2(0.0f, 250.0f);
	const glm::vec2 S = glm::vec2(keyIdToPositionX(ss.start.id), -300.0f);
	const glm::vec2 E = glm::vec2(keyIdToPositionX(ss.end.id), -300.0f);
	const float R = 150.0f;
	
	if(ss.type == "-"){
		return fabs(keyIdToPositionX(ss.start.id) - keyIdToPositionX(ss.end.id));
	}
	else if(ss.type == "L"){
		float len = keyIdToPositionX(ss.start.id) - keyIdToPositionX(ss.end.id);
		if(len < 0.0f) len = 8.0f * 150.0f + len;
		return pi * (len + 200.0f) / 2.0f;
	}
	else if(ss.type == "R"){
		float len = keyIdToPositionX(ss.end.id) - keyIdToPositionX(ss.start.id);
		if(len < 0.0f) len = 8.0f * 150.0f + len;
		return pi * (len + 200.0f) / 2.0f;
	}
	else if(ss.type == "v"){
		return glm::length(C-S) + glm::length(E-C);
	}
	else if(ss.type == "p"){
		return getSlidePQPPQQSegLength('p', C, S, E, R, cache);
	}
	else if(ss.type == "q"){
		return getSlidePQPPQQSegLength('q', C, S, E, R, cache);
	}
	else if(ss.type == "pp"){
		glm::vec2 O = glm::vec2(keyIdToPositionX((1.0f * ss.start.id + 1.0f * ss.end.id) / 2.0f), -50.0f);
		return getSlidePQPPQQSegLength('p', O, S, E, R, cache);
	}
	else if(ss.type == "qq"){
		glm::vec2 O = glm::vec2(keyIdToPositionX((1.0f * ss.start.id + 1.0f * ss.end.id) / 2.0f), -50.0f);
		return getSlidePQPPQQSegLength('q', O, S, E, R, cache);
	}
	else if(ss.type == "s" || ss.type == "z"){
		glm::vec2 O = glm::vec2(keyIdToPositionX((1.0f * ss.start.id + 1.0f * ss.end.id) / 2.0f), -200.0f);
		return glm::length(O - S) * 2.0f + 200.0f;
	}
	else if(ss.type == "w"){
		return 600.0f;
	}
	else if(ss.type == "N"){
		return 0.0f;
	}
}
void addDragSnake(int sid, int eid, int segDrag, phifrac curTime, phifrac stride){
	phigros_note_t drag;
	drag.type = 4;
	drag.speed = 1.0f;
	for(int i = 0; i < segDrag; i++){
		drag.startTime = drag.endTime = curTime;
		drag.positionX = glm::mix(
			keyIdToPositionX(sid),
			keyIdToPositionX(eid),
			1.0f * i / (1.0f * (segDrag-1))
		);
		phigrosChart.judgeLineList[0].notes.push_back(drag);
		curTime += stride;
	}
}
void addDragLine(int totDrag, glm::vec2 S, glm::vec2 E, phifrac curTime, phifrac stride, float visibleTime){
	phigros_note_t drag;
	drag.type = 4;
	drag.speed = 1.0f;
	drag.visibleTime=visibleTime;
	for(int i = 0; i < totDrag; i++){
		float ratio = 1.0f * i / (1.0f * (totDrag-1));
		drag.startTime = drag.endTime = curTime;
		drag.positionX = glm::mix(S.x, E.x, ratio);
		drag.yOffset = 300.0f + glm::mix(S.y, E.y, ratio);
		phigrosChart.judgeLineList[21].notes.push_back(drag);
		curTime += stride;
	} 
}

int normalizeKeyId(int x){
	return (x+7)%8+1;
}

void addNotesForSegPQPPQQC(slidePQPPQQSegCache_t& cache, int totDrag, phifrac curTime, phifrac stride, float R, float visibleTime){
	phigros_note_t drag;
	drag.type = 4;
	drag.speed = 1.0f;
	drag.visibleTime=visibleTime;
	for(int i=0; i < totDrag; i++){
		float ratio = 1.0f * i / (1.0f * (totDrag-1));
		drag.startTime = drag.endTime = curTime;
		float radAngle = glm::mix(cache.radSAngle, cache.radEAngle, ratio);
		drag.positionX = cache.O.x + R * glm::cos(radAngle);
		drag.yOffset = 300.0f + cache.O.y + R * glm::sin(radAngle);
		phigrosChart.judgeLineList[21].notes.push_back(drag);
		curTime += stride;
	}
	
}
void addNotesForSegPQPPQQ(char type, slidePQPPQQSegCache_t& cache, int totDrag, phifrac curTime, phifrac stride, phifrac segTime, float visibleTime){
	const float R = 150.0f;
	glm::vec2 M1 = cache.O + R * glm::vec2(glm::cos(cache.radSAngle), glm::sin(cache.radSAngle));
	glm::vec2 M2 = cache.O + R * glm::vec2(glm::cos(cache.radEAngle), glm::sin(cache.radEAngle));
	float ratioS = cache.sLen / cache.totLen;
	float ratioC = cache.cLen / cache.totLen;
	int dS = max(2, (int)(totDrag * ratioS));
	int dC = max(2, (int)(totDrag * ratioC));
	int dE = max(2, (int)(totDrag - dS - dC)); 
	stride = segTime / (dS + dC + dE - 1);
	addDragLine(dS, cache.S, M1, curTime, stride, visibleTime);
	curTime += stride * dS;
	if(type == 'p'){
		if(cache.radEAngle <= cache.radSAngle) cache.radEAngle += glm::radians(360.0f);
	}
	else{
		if(cache.radEAngle >= cache.radSAngle) cache.radSAngle += glm::radians(360.0f);
	}
	addNotesForSegPQPPQQC(cache, dC, curTime, stride, R, visibleTime);
	curTime += stride * dC;
	addDragLine(dE, M2, cache.E, curTime, stride, visibleTime);
}
void addNotesForSeg(slide_seg_t& ss, phifrac curTime, phifrac segTime, slidePQPPQQSegCache_t& cache){
	phifrac stride;
	phigros_note_t drag;
	drag.type = 4;
	drag.visibleTime = 2;
	
	int totDrag = max(2LL,normalize(segTime / phifrac(1, 8)).integer + 1);
	stride = segTime / (totDrag-1);
	
	if(ss.type == "-"){
		addDragSnake(ss.start.id, ss.end.id, totDrag, curTime, stride);
	}
	else if(ss.type == "L"){
		int sid=ss.start.id, eid=ss.end.id;
		if(sid <= eid) eid -= 8; 
		float s = keyIdToPositionX(sid);
		float e = keyIdToPositionX(eid);
		float mid = (s + e) / 2.0f;
		drag.speed = 1.0f;
		
		for(int i = 0; i < totDrag; i++){
			drag.startTime = drag.endTime = curTime;
			float ratio = 1.0f * i / (1.0f * (totDrag-1));
			drag.yOffset = 200.0f * glm::sin(ratio * glm::radians(180.0f));
			drag.positionX = mid + (mid - e) * glm::cos(ratio * glm::radians(180.0f));
			if(drag.positionX < -600.0f) drag.positionX += 8 * 150.0f;
			phigrosChart.judgeLineList[21].notes.push_back(drag);
			curTime += stride; 
		}
	}
	else if(ss.type == "R"){
		int sid=ss.start.id, eid=ss.end.id;
		if(sid >= eid) eid += 8; 
		float s = keyIdToPositionX(sid);
		float e = keyIdToPositionX(eid);
		float mid = (s + e) / 2.0f;
		drag.speed = 1.0f;
		
		for(int i = 0; i < totDrag; i++){
			drag.startTime = drag.endTime = curTime;
			float ratio = 1.0f * i / (1.0f * (totDrag-1));
			drag.yOffset = 200.0f * glm::sin(ratio * glm::radians(180.0f));
			drag.positionX = mid + (mid - e) * glm::cos(ratio * glm::radians(180.0f));
			if(drag.positionX > 600.0f) drag.positionX -= 8 * 150.0f;
			phigrosChart.judgeLineList[21].notes.push_back(drag);
			curTime += stride;
		}
	}
	else if(ss.type == "v"){
		glm::vec2 S = glm::vec2(keyIdToPositionX(ss.start.id), -300.0f); 
		glm::vec2 E = glm::vec2(keyIdToPositionX(ss.end.id), -300.0f); 
		glm::vec2 C = glm::vec2(0.0f, 250.0f);
		float ratio = glm::length(C-S) / (glm::length(E-C) + glm::length(C-S));
		int totDrag1 = max(2, int(ratio * totDrag));
		int totDrag2 = max(2, totDrag - totDrag1);
		stride = segTime / (totDrag1 + totDrag2 - 1);
		addDragLine(totDrag1, S, C, curTime, stride, drag.visibleTime); 
		curTime += stride * totDrag1;
		addDragLine(totDrag2, C, E, curTime, stride, drag.visibleTime); 
	}
	else if(ss.type == "s"){
		glm::vec2 S = glm::vec2(keyIdToPositionX(ss.start.id), -300.0f); 
		glm::vec2 E = glm::vec2(keyIdToPositionX(ss.end.id), -300.0f); 
		float mid = (keyIdToPositionX(ss.start.id) + keyIdToPositionX(ss.end.id)) / 2;
		glm::vec2 M1= glm::vec2(mid, 0.0f);
		if(ss.start.id < ss.end.id) M1.y = -400.0f;
		else M1.y = -200.0f;
		glm::vec2 M2= M1;
		M2.y = -600.0f - M1.y;
		
		float l1 = glm::length(M1 - S);
		float l2 = 200.0f;
		float totLength = l1 * 2 + l2;
		float ratio = l1 / totLength;
		int d1 = max(2, int(totDrag * ratio));
		int d2 = max(2, totDrag - 2 * d1);
		stride = segTime / (d1 * 2 + d2 - 1);
		addDragLine(d1, S, M1, curTime, stride, drag.visibleTime);
		curTime += stride * d1;
		addDragLine(d2, M1, M2, curTime, stride, drag.visibleTime);
		curTime += stride * d2;
		addDragLine(d1, M2, E, curTime, stride, drag.visibleTime);
	}
	else if(ss.type == "z"){
		glm::vec2 S = glm::vec2(keyIdToPositionX(ss.start.id), -300.0f); 
		glm::vec2 E = glm::vec2(keyIdToPositionX(ss.end.id), -300.0f); 
		float mid = (keyIdToPositionX(ss.start.id) + keyIdToPositionX(ss.end.id)) / 2;
		glm::vec2 M1= glm::vec2(mid, 0.0f);
		if(ss.start.id > ss.end.id) M1.y = -400.0f;
		else M1.y = -200.0f;
		glm::vec2 M2= M1;
		M2.y = -600.0f - M1.y;
		
		float l1 = glm::length(M1 - S);
		float l2 = 200.0f;
		float totLength = l1 * 2 + l2;
		float ratio = l1 / totLength;
		int d1 = totDrag * ratio;
		int d2 = totDrag - 2 * d1;
		stride = segTime / (d1 * 2 + d2 - 1);
		addDragLine(d1, S, M1, curTime, stride, drag.visibleTime);
		curTime += stride * d1;
		addDragLine(d2, M1, M2, curTime, stride, drag.visibleTime);
		curTime += stride * d2;
		addDragLine(d1, M2, E, curTime, stride, drag.visibleTime);
	}
	else if(ss.type == "w"){
		phifrac curTime1 = curTime, curTime2 = curTime;
		addDragSnake(ss.start.id, normalizeKeyId(ss.end.id-1), totDrag, curTime1, stride);
		addDragSnake(ss.start.id, normalizeKeyId(ss.end.id), totDrag, curTime, stride);
		addDragSnake(ss.start.id, normalizeKeyId(ss.end.id+1), totDrag, curTime2, stride);
	}
	else if(ss.type == "p" || ss.type == "q" || ss.type == "pp" || ss.type == "qq"){
		addNotesForSegPQPPQQ(ss.type[0], cache, totDrag, curTime, stride, segTime, drag.visibleTime);
	}
}


void config_1(maimai_chart_data_t& crt){ 
	ifstream cfgi("config/1.txt");
	float mX, mY;
	int alpha;
	float rotate, speed;
	while(cfgi >> mX){
		cfgi >> mY >> alpha >> rotate >> speed;
		phigros_judgeline_t jl;
		phigros_event_layer_t el; 
		
		phigros_event_t<int, 0, 0> aEvent;
		phigros_event_t<float, 0, 1> mXEvent, mYEvent, rEvent;
		phigros_event_t<float, 10, 1> spdEvent;
		
		mXEvent.start = mXEvent.end = mX;
		mXEvent.startTime = 0;
		mXEvent.endTime = crt.endTime;
		el.moveXEvents.push_back(mXEvent);
		
		mYEvent.start = mYEvent.end = mY;
		mYEvent.startTime = 0;
		mYEvent.endTime = crt.endTime;
		el.moveYEvents.push_back(mYEvent);
		
		aEvent.start = aEvent.end = alpha;
		aEvent.startTime = 0;
		aEvent.endTime = crt.endTime;
		el.alphaEvents.push_back(aEvent);
		
		rEvent.start = rEvent.end = rotate;
		rEvent.startTime = 0;
		rEvent.endTime = crt.endTime;
		el.rotateEvents.push_back(rEvent);
		
		if(speed >= 0){
			spdEvent.start = spdEvent.end = speed;
			spdEvent.startTime = 0;
			spdEvent.endTime = crt.endTime;
			el.speedEvents.push_back(spdEvent);
		}
		
		jl.eventLayers.push_back(el);
		phigrosChart.judgeLineList.push_back(jl);
	}
}
void translate_1(maimai_chart_data_t& crt){ // param: maimaiCharts[...]
	config_1(crt);
//	maimai_note_t& mNote; // refer
	for(maimai_note_t& mNote: crt.notes){
		phigros_note_t pNote;
		if(mNote.type == 0){
			int keyId=mNote.segs[0].start.id;
			if(!mNote.isHold){ // tap
				pNote.type = 1;
				pNote.startTime = pNote.endTime = mNote.start;
			}
			else{ // hold
				pNote.type = 2;
				pNote.startTime = mNote.start;
				pNote.endTime   = mNote.end;
			}
			pNote.positionX = keyIdToPositionX(keyId);
			phigrosChart.judgeLineList[0].notes.push_back(pNote);
			if(mNote.isHeadBreak){ // break
				phigros_note_t drag;
				drag.type = 4;
				drag.positionX = keyIdToPositionX(keyId);
				if(!mNote.isHold){
					drag.startTime = drag.endTime = mNote.start + phifrac(1, 16);
					phigrosChart.judgeLineList[0].notes.push_back(drag);
				}
				else{
					for(phifrac t = mNote.start; t <= mNote.end; t += phifrac(0.5f)){
						drag.startTime = drag.endTime = t;
						phigrosChart.judgeLineList[0].notes.push_back(drag);
					}
				}
			}
		}
		else if(mNote.type == 1){ // touch
			int horiLineId, vertLineId;
			float horiLineX, vertLineX;
			char alpha = mNote.segs[0].start.alpha;
			int id = mNote.segs[0].start.id;
			if(alpha == 'C'){
				horiLineId = 19;
				horiLineX  = 0.0f;
				vertLineId = 12;
				vertLineX  = 250.0f;
			}
			else if(alpha == 'A'){
				horiLineId = 20;
				horiLineX  = keyIdToPositionX(id);
				vertLineId = id;
				vertLineX  = -300.0f;
			}
			else if(alpha == 'D'){
				horiLineId = 20;
				horiLineX  = keyIdToPositionX(id + 0.5f);
				vertLineId = id + 8;
				vertLineX  = -300.0f;
			}
			else if(alpha == 'E'){
				horiLineId = 17;
				horiLineX  = keyIdToPositionX(id + 0.5f);
				vertLineId = id + 8;
				vertLineX  = -100.0f;
			}
			else if(alpha == 'B'){
				horiLineId = 18;
				horiLineX  = keyIdToPositionX(id);
				vertLineId = id;
				vertLineX  = 100.0f;
			}
			phigros_note_t drag;
			drag.type = 4;
			drag.startTime = drag.endTime = mNote.start;
			drag.visibleTime = 0.5f;
			// hori
			drag.above = 1;
			drag.positionX = horiLineX;
			phigrosChart.judgeLineList[horiLineId].notes.push_back(drag);
			drag.above = 0;
			phigrosChart.judgeLineList[horiLineId].notes.push_back(drag);
			// vert
			drag.above = 1;
			drag.positionX = vertLineX;
			phigrosChart.judgeLineList[vertLineId].notes.push_back(drag);
			drag.above = 0;
			phigrosChart.judgeLineList[vertLineId].notes.push_back(drag);
			if(mNote.isHold){
				phigros_note_t hold;
				hold.type = 2;
				hold.startTime = mNote.start;
				hold.endTime = mNote.end;
//				hold.speed = 0.5f;
				hold.positionX = horiLineX;
				hold.visibleTime = 0.5f;
				phigrosChart.judgeLineList[horiLineId].notes.push_back(hold);
			}
		} 
		else if(mNote.type == 2){ // slide
			
			// ----------------------
			//  star in maimai -> tap
			phigros_note_t tap;
			tap.type = 1;
			tap.startTime = tap.endTime = mNote.start;
			tap.positionX = keyIdToPositionX(mNote.segs[0].start.id);
			phigrosChart.judgeLineList[0].notes.push_back(tap);
			
			// ----------------------
			//  calc for ratio
			slide_seg_t ss;
			float totLength = 0.0f;
			phifrac totTime = mNote.end - mNote.start - mNote.slideDelta;
			vector<float> lengthCache;
			vector<slidePQPPQQSegCache_t> angleCache;
			for(slide_seg_t& ss: mNote.segs){
				slidePQPPQQSegCache_t cache;
				lengthCache.push_back(getSlideSegLength(ss, cache));
				totLength += lengthCache[lengthCache.size()-1];
				angleCache.push_back(cache);
			}
			
			// ----------------------
			//  main
			phifrac curTime = mNote.start + mNote.slideDelta;
			for(int i = 0; i + 1 < mNote.segs.size(); i++){
				slide_seg_t& ss = mNote.segs[i];
				phifrac segTime = phifrac(lengthCache[i] / totLength) * totTime;
				addNotesForSeg(ss, curTime, segTime, angleCache[i]);
				curTime = curTime + segTime;
			}
		} 
	}
}

int main(){
	string s,origPath;
	ofstream infoOut("output/info.txt");
	
	cout << "要解析的谱面：";
	getline(cin,origPath);
	
	s = get_file(origPath.c_str());
	
	cout << "谱面 ID：";
	getline(cin,metadata.id);
	
	cout << "音乐文件名：";
	getline(cin,metadata.song);
	
	cout << "曲绘文件名：";
	getline(cin,metadata.picture);
	
	cout << "文件解析中. . . ";
	decodeSimai(s);
	
	cout << "\r文件解析成功。检测到以下难度的谱面。请输入要转换的谱面的编号：\n";
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
		cout << "Type: " << g.type << " Start: " << g.start << " End: " << g.end << " Key: " << g.segs[0].start.alpha << g.segs[0].start.id << "\n";
	}
	
	cout << "文件转换中. . . ";
	translate_1(maimaiCharts[targetDiff]);
	
	cout << "\r文件转换成功，正在写入文件. . . ";
	
	ofstream ofs(("output/"+metadata.chart).c_str());
	ofs << "{\n";
	maimaiCharts[targetDiff].bpmlist.outputPhiJSON(ofs, 3);
	metadata.outputPhiJSON(ofs, 3, targetDiff);
	phigrosChart.outputPhiJSON(ofs, 3);
	ofs << "}";
	
	cout << "\r文件写入完成。转换流程结束        \n";
	
//	while(1) cout << "";
}
