#include "KoreanText.h"

#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

int main(){
    // [받침 감별기]
    const std::vector<std::pair<std::string,bool>> batchimCases={
        {"가",false},
        {"나",false},
        {"사과",false},
        {"울티마",false},
        {"왕",true},
        {"성",true},
        {"칼",true},
        {"기둥",true},
        {"표지판",true},
        {"무덤",true},
        {"왕   ",true},
        {"가\t",false},
        {"",false},
        {"ABC",false},
        {"왕!",false},
    };

    for(const auto& [text,expected]:batchimCases){
        const bool got=받침_감별기(text);
        if(got!=expected){
            std::cerr<<"받침 감별기 FAIL: ["<<text<<"] expected "
                     <<expected<<" got "<<got<<"\n";
            return 1;
        }
    }

    // 종성 번호: 0=없음, 8=ㄹ. @으로 예외 검증에 직접 사용한다.
    if(종성_번호("돌팔매")!=0){
        std::cerr<<"종성 번호 FAIL: 돌팔매\n";
        return 1;
    }
    if(종성_번호("칼")!=8){
        std::cerr<<"종성 번호 FAIL: 칼 should have ㄹ(8)\n";
        return 1;
    }
    if(종성_번호("단검")<=0 || 종성_번호("단검")==8){
        std::cerr<<"종성 번호 FAIL: 단검\n";
        return 1;
    }

    // 조사 선택기 단위 테스트.
    const std::vector<std::tuple<std::string,std::string,std::string>> particleCases={
        {"경비병","이","이"},
        {"광대","이","가"},
        {"왕","은","은"},
        {"광대","은","는"},
        {"활","을","을"},
        {"돌팔매","을","를"},
        {"단검","으로","으로"},
        {"돌팔매","으로","로"},
        {"칼","으로","로"}, // ㄹ 받침 예외
    };

    for(const auto& [word,marker,expected]:particleCases){
        const std::string got=조사_선택(word,marker);
        if(got!=expected){
            std::cerr<<"조사 선택 FAIL: ["<<word<<"] @"<<marker
                     <<" expected "<<expected<<" got "<<got<<"\n";
            return 1;
        }
    }

    // 실제 조합형 출력 문법.
    const std::vector<std::pair<std::string,std::string>> templateCases={
        {"[단검]@으로 공격","단검으로 공격"},
        {"[돌팔매]@으로 공격","돌팔매로 공격"},
        {"[칼]@으로 공격","칼로 공격"},
        {"[경비병]@이 공격합니다","경비병이 공격합니다"},
        {"[광대]@이 공격합니다","광대가 공격합니다"},
        {"[왕]@은 기다립니다","왕은 기다립니다"},
        {"[광대]@은 기다립니다","광대는 기다립니다"},
        {"[활]@을","활을"},
        {"[돌팔매]@을","돌팔매를"},
        {"[경비병]@이 [광대]@으로 공격합니다","경비병이 광대로 공격합니다"},
        {"앞 [단검]@으로 공격 뒤","앞 단검으로 공격 뒤"},
    };

    for(const auto& [input,expected]:templateCases){
        const std::string got=조사_처리기(input);
        if(got!=expected){
            std::cerr<<"조사 처리 FAIL: ["<<input<<"] expected ["
                     <<expected<<"] got ["<<got<<"]\n";
            return 1;
        }
    }

    std::cout<<"M98 받침/조사 처리 test OK\n";
    return 0;
}
