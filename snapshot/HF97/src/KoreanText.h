#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// -----------------------------------------------------------------------------
// [받침 감별기] / Korean particle helpers
// -----------------------------------------------------------------------------
//
// 이 파일은 조합형 한글 문장을 만들 때 사용하는 공용 함수 모음이다.
//
// 지원하는 조사 표기:
//   [단어]@이    -> 받침 있음: 이 / 받침 없음: 가
//   [단어]@은    -> 받침 있음: 은 / 받침 없음: 는
//   [단어]@을    -> 받침 있음: 을 / 받침 없음: 를
//   [단어]@으로  -> 일반 받침: 으로 / 받침 없음 또는 ㄹ 받침: 로
//
// 예:
//   조사_처리기("[단검]@으로 공격")       -> "단검으로 공격"
//   조사_처리기("[돌팔매]@으로 공격")     -> "돌팔매로 공격"
//   조사_처리기("[칼]@으로 공격")         -> "칼로 공격"
//   조사_처리기("[경비병]@이 공격합니다") -> "경비병이 공격합니다"
//   조사_처리기("[광대]@이 공격합니다")   -> "광대가 공격합니다"
//
// 프로젝트 규칙상 @ 조사 표기는 한글 단어 뒤에서만 사용한다.

// text의 마지막 유효 UTF-8 code point를 읽는다.
// UI 조립 과정에서 붙을 수 있는 trailing ASCII whitespace는 무시한다.
// 성공하면 true와 code point를 반환하고, malformed UTF-8/빈 문자열이면 false.
inline bool 마지막_코드포인트(const std::string& text,char32_t& cp){
    if(text.empty()) return false;

    std::size_t end=text.size();
    while(end>0){
        const unsigned char c=static_cast<unsigned char>(text[end-1]);
        if(c==' ' || c=='\t' || c=='\r' || c=='\n') --end;
        else break;
    }
    if(end==0) return false;

    std::size_t start=end-1;
    while(start>0 &&
          (static_cast<unsigned char>(text[start])&0xC0u)==0x80u){
        --start;
    }

    const unsigned char b0=static_cast<unsigned char>(text[start]);
    std::size_t len=0;
    cp=0;

    if((b0&0x80u)==0){
        cp=b0;
        len=1;
    }else if((b0&0xE0u)==0xC0u){
        cp=b0&0x1Fu;
        len=2;
    }else if((b0&0xF0u)==0xE0u){
        cp=b0&0x0Fu;
        len=3;
    }else if((b0&0xF8u)==0xF0u){
        cp=b0&0x07u;
        len=4;
    }else{
        return false;
    }

    if(start+len!=end) return false;

    for(std::size_t i=1;i<len;++i){
        const unsigned char bx=static_cast<unsigned char>(text[start+i]);
        if((bx&0xC0u)!=0x80u) return false;
        cp=(cp<<6)|(bx&0x3Fu);
    }
    return true;
}

// 마지막 완성형 한글 음절의 종성 번호를 반환한다.
//   -1 : 마지막 유효 문자가 완성형 한글이 아님
//    0 : 받침 없음
// 1~27 : 받침 있음
//
// Unicode 완성형 한글은 U+AC00..U+D7A3에 있고,
// 각 초성/중성 조합마다 [받침 없음 + 27개 종성]의 28개가 연속 배치된다.
inline int 종성_번호(const std::string& text){
    char32_t cp=0;
    if(!마지막_코드포인트(text,cp)) return -1;

    constexpr char32_t HANGUL_FIRST=0xAC00;
    constexpr char32_t HANGUL_LAST =0xD7A3;
    if(cp<HANGUL_FIRST || cp>HANGUL_LAST) return -1;

    return static_cast<int>((cp-HANGUL_FIRST)%28u);
}

// [받침 감별기]
// 특정 UTF-8 한글 string의 마지막 글자에 받침이 있는지 판별한다.
// 마지막 유효 문자가 완성형 한글이 아니면 false.
inline bool 받침_감별기(const std::string& text){
    return 종성_번호(text)>0;
}

// marker에는 '@'를 제외한 "이", "은", "을", "으로"를 넘긴다.
// 지원하지 않는 marker는 원문 확인이 가능하도록 "@"+marker 그대로 반환한다.
inline std::string 조사_선택(const std::string& word,const std::string& marker){
    const int jong=종성_번호(word);
    if(jong<0) return "@"+marker;

    if(marker=="이")
        return jong>0 ? "이" : "가";

    if(marker=="은")
        return jong>0 ? "은" : "는";

    if(marker=="을")
        return jong>0 ? "을" : "를";

    if(marker=="으로"){
        // ㄹ 종성은 Unicode 종성 배열의 8번이다.
        // 받침 없음(0)과 ㄹ 받침(8)은 모두 "로", 그 외 받침은 "으로".
        return (jong==0 || jong==8) ? "로" : "으로";
    }

    return "@"+marker;
}

// [단어]@조사 형태의 조합형 문자열을 실제 한국어 문자열로 변환한다.
//
// 대괄호는 조사 판정 대상의 경계를 명확하게 지정하기 위한 문법이며 출력되지 않는다.
// 지원하는 token은 @으로 / @은 / @을 / @이 이다. @으로를 먼저 검사해 prefix 충돌을 피한다.
//
// 지원하지 않거나 불완전한 패턴은 가능한 한 원문 그대로 보존한다.
inline std::string 조사_처리기(const std::string& text){
    std::string out;
    std::size_t pos=0;

    while(pos<text.size()){
        const std::size_t open=text.find('[',pos);
        if(open==std::string::npos){
            out.append(text,pos,std::string::npos);
            break;
        }

        out.append(text,pos,open-pos);

        const std::size_t close=text.find(']',open+1);
        if(close==std::string::npos || close+1>=text.size() || text[close+1]!='@'){
            // 조사 문법이 아니면 '['를 평문으로 취급하고 계속 찾는다.
            out.push_back('[');
            pos=open+1;
            continue;
        }

        const std::string word=text.substr(open+1,close-open-1);
        const std::size_t markerPos=close+2;

        std::string marker;
        std::size_t markerBytes=0;

        // UTF-8 byte 문자열 비교. 긴 token부터 확인한다.
        static const std::string EU_RO="으로";
        static const std::string EUN="은";
        static const std::string EUL="을";
        static const std::string I_GA="이";

        if(text.compare(markerPos,EU_RO.size(),EU_RO)==0){
            marker=EU_RO;
            markerBytes=EU_RO.size();
        }else if(text.compare(markerPos,EUN.size(),EUN)==0){
            marker=EUN;
            markerBytes=EUN.size();
        }else if(text.compare(markerPos,EUL.size(),EUL)==0){
            marker=EUL;
            markerBytes=EUL.size();
        }else if(text.compare(markerPos,I_GA.size(),I_GA)==0){
            marker=I_GA;
            markerBytes=I_GA.size();
        }else{
            // 알 수 없는 @ token이면 전체 bracketed word를 그대로 보존한다.
            out.append(text,open,close-open+2); // "[word]@"
            pos=markerPos;
            continue;
        }

        out+=word;
        out+=조사_선택(word,marker);
        pos=markerPos+markerBytes;
    }

    return out;
}


// User-approved transport names. Keep these centralized so the Transport shop,
// Board messages, and Ztats inventory use exactly the same localized nouns.
// User-approved transport names including the Time Machine inventory label.
inline const char* 운송수단_이름_한글(int index){
    static constexpr const char* NAMES[8]={
        "Foot", "말", "마차", "뗏목", "범선", "비행선", "우주 왕복선", "타임머신"
    };
    return (index>=0 && index<8) ? NAMES[index] : "";
}


// User-approved gem names, in the original OUT.EXE inventory order.
inline const char* 보석_이름_한글(int index){
    static constexpr const char* NAMES[4]={
        "붉은색 보석", "녹색 보석", "푸른색 보석", "흰색 보석"
    };
    return (index>=0 && index<4) ? NAMES[index] : "";
}


// User-approved Ultima I weapon translations.
// Keep this table centralized so Attack/Ready/shop/loot text cannot drift.
inline const char* 무기_이름_한글(int index){
    static constexpr const char* NAMES[16]={
        "맨손", "단검", "철퇴", "도끼", "밧줄과 못", "검", "대검", "활과 화살",
        "수호부", "지팡이", "마법봉", "삼각검", "권총", "광선검", "페이저", "블라스터"
    };
    return (index>=0 && index<16) ? NAMES[index] : "";
}


// User-approved generic replacement for "Attack with [weapon]".
// Never special-case Dagger: every weapon goes through the same @으로 grammar.
inline std::string 무기_공격_문구_한글(int index){
    const std::string weapon=무기_이름_한글(index);
    return 조사_처리기("["+weapon+"]@으로 공격하기");
}


// User-approved local NPC translations.
// `inTown` is required because OUT.EXE uses the same actor tile for
// Bard/Jester and Wench/Lecher depending on the local-map context.
inline const char* 인물_이름_한글(std::uint16_t tile,bool inTown){
    switch(tile){
        case 0x14:return "국왕";      // King
        case 0x11:return "경비병";    // Guard
        case 0x15:return "상인";      // Merchant
        case 0x16:return "공주";      // Princess
        case 0x13:return inTown ? "음유시인" : "광대"; // Bard / Jester
        case 0x32:return inTown ? "꽃뱀" : "제비";     // Wench / Lecher
        default:return "";
    }
}


// User-approved castle-name translations.
inline const char* 성_이름_한글(const std::string& english){
    if(english=="The Castle of Lord British") return "로드 브리티시성";
    if(english=="The Castle of the Lost King") return "잊힌 왕의 성";
    if(english=="The Castle Barataria") return "바라타리아성";
    if(english=="The Castle Rondorin") return "론도린성";
    if(english=="The Castle of Olympus") return "올림푸스성";
    if(english=="The Black Dragon's Castle") return "흑룡성";
    if(english=="The White Dragon's Castle") return "백룡성";
    if(english=="The Castle of Shamino") return "샤미노성";
    return nullptr;
}


inline std::string 공주_이름_한글(const std::string& original){
    if(original=="Julia") return "줄리아";
    if(original=="Cassandra") return "카산드라";
    if(original=="Marsha") return "마샤";
    if(original=="Donna") return "돈나";
    if(original=="Kristen") return "크리스턴";
    if(original=="Beth") return "베스";
    if(original=="Lori") return "로리";
    if(original=="Dianne") return "다이앤";
    // OUT.EXE has an eighth Princess, Julia (castle id 33). The user has not
    // supplied a Korean spelling for her yet, so preserve the oracle name.
    return original;
}