#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

inline std::size_t gameplayUtf8Next(const std::string& s,std::size_t pos){
    if(pos>=s.size()) return s.size();
    const unsigned char c=static_cast<unsigned char>(s[pos]);
    std::size_t len=1;
    if((c&0xE0u)==0xC0u) len=2;
    else if((c&0xF0u)==0xE0u) len=3;
    else if((c&0xF8u)==0xF0u) len=4;
    return std::min(s.size(),pos+len);
}

// Wrap text by rendered pixel width without splitting a UTF-8 code point.
// Prefer the last ASCII space that fits; if a single token is too wide,
// fall back to a UTF-8-safe hard split. Leading spaces on continuation rows
// are discarded because the DOS command window resumes at column 1.
template<class Measure>
std::vector<std::string> wrapGameplayTextMeasured(
    const std::string& text,int maxWidth,Measure measure){

    std::vector<std::string> lines;
    if(text.empty() || maxWidth<=0) return lines;

    std::size_t paragraphStart=0;
    while(paragraphStart<=text.size()){
        const std::size_t newline=text.find('\n',paragraphStart);
        const std::size_t paragraphEnd=
            (newline==std::string::npos)?text.size():newline;
        std::string rest=text.substr(paragraphStart,paragraphEnd-paragraphStart);

        if(rest.empty()){
            lines.push_back("");
        }else{
            while(!rest.empty()){
                if(measure(rest)<=maxWidth){
                    lines.push_back(rest);
                    break;
                }

                std::size_t fit=0;
                for(std::size_t pos=0;pos<rest.size();){
                    const std::size_t next=gameplayUtf8Next(rest,pos);
                    if(measure(rest.substr(0,next))>maxWidth) break;
                    fit=next;
                    pos=next;
                }

                // Defensive progress even if a glyph itself exceeds maxWidth.
                if(fit==0) fit=gameplayUtf8Next(rest,0);

                std::size_t consumed=0;

                // If the maximal fitting prefix ends exactly at a word
                // boundary, keep the entire fitting word.  Otherwise prefer
                // the last space inside that prefix.
                if(fit<rest.size() && rest[fit]==' '){
                    lines.push_back(rest.substr(0,fit));
                    consumed=fit+1;
                    while(consumed<rest.size() && rest[consumed]==' ') ++consumed;
                }else{
                    std::size_t breakAt=std::string::npos;
                    if(fit>0){
                        breakAt=rest.rfind(' ',fit-1);
                        if(breakAt==0) breakAt=std::string::npos;
                    }

                    if(breakAt!=std::string::npos){
                        lines.push_back(rest.substr(0,breakAt));
                        consumed=breakAt+1;
                        while(consumed<rest.size() && rest[consumed]==' ') ++consumed;
                    }else{
                        lines.push_back(rest.substr(0,fit));
                        consumed=fit;
                    }
                }
                rest.erase(0,consumed);
            }
        }

        if(newline==std::string::npos) break;
        paragraphStart=newline+1;
    }
    return lines;
}