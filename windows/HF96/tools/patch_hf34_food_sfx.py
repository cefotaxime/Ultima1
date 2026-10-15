from pathlib import Path
p=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_1.0K_Source_HF34\src\main.cpp")
s=p.read_text(encoding="utf-8")
old='''                    }else if(beforeStage==TradeStage::FoodQuantity && done){
                        if(!beforeLine.empty()) log.pushCommand(beforeLine);
                        pushMessages(m);
                        finishCommand();
                    }else if(beforeStage==TradeStage::Message && done){'''
new='''                    }else if(beforeStage==TradeStage::FoodQuantity && done){
                        if(!beforeLine.empty()) log.pushCommand(beforeLine);
                        pushMessages(m);
                        if(noiseEnabled && shopCantAfford)
                            beginCommandSfxHold(SfxEvent::LocalBlocked,1);
                        else
                            finishCommand();
                    }else if(beforeStage==TradeStage::Message && done){'''
assert s.count(old)==1, s.count(old)
p.write_text(s.replace(old,new),encoding="utf-8")
print("patched")