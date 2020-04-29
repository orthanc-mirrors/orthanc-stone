var SendMessageToCpp: Function = null;
export var TestWasmIntegratedModule : any;

import * as TestStoneCodeGen from './build-wasm/TestStoneCodeGen_generated'

/*
+--------------------------------------------------+
|  install emscripten handlers                     |
+--------------------------------------------------+
*/

// (<any> window).Module = {
//   preRun: [ 
//     function() {
//     console.log('Loading the Stone Framework using WebAssembly');
//     }
//   ],
//   postRun: [ 
//     function()  {
//     // This function is called by ".js" wrapper once the ".wasm"
//     // WebAssembly module has been loaded and compiled by the
//     // browser
//     console.log('WebAssembly is ready');
//     // window.SendMessageToCpp = (<any> window).Module.cwrap('SendMessageToCpp', 'string', ['string']);
//     // window.SendFreeTextToCpp = (<any> window).Module.cwrap('SendFreeTextToCpp', 'string', ['string']);
//     }
//   ],
//   print: function(text : string) {
//     console.log(text);
//   },
//   printErr: function(text : string) {
//     console.error(text);
//   },
//   totalDependencies: 0
// };

/*
+--------------------------------------------------+
|  install handlers                                |
+--------------------------------------------------+
*/
document.querySelectorAll(".TestWasm-button").forEach((e) => {
  (e as HTMLButtonElement).addEventListener("click", () => {
    ButtonClick(e.attributes["tool-selector"].value);
  });
});

/*
+--------------------------------------------------+
|  define stock messages                           |
+--------------------------------------------------+
*/
let schemaText: string = null;
fetch("testTestStoneCodeGen.yaml").then(function(res) {return res.text();}).then(function(text) {schemaText = text;});

let stockSerializedMessages = new Map<string,string>();
stockSerializedMessages["Test CppHandler message2"] = null;
fetch("cppHandler_test_Message2.json").then(function(res) {return res.text();}).then(function(text) {stockSerializedMessages["Test CppHandler message2"] = text;});

stockSerializedMessages["Test 2"] = ` {
  "type" : "TestStoneCodeGen.Message1",
  "value" : {
    "memberInt32" : -987,
    "memberString" : "Salomé",
    "memberEnumMonth" : "March",
    "memberBool" : true,
    "memberFloat32" : 0.1,
    "memberFloat64" : -0.2,
    "extraMember" : "don't care"
  }
}`;
stockSerializedMessages["Test 3"] = "Test 3 stock message sdfsfsdfsdf";
stockSerializedMessages["Test 4"] = "Test 4 stock message 355345345";
stockSerializedMessages["Test 5"] = "Test 5 stock message 34535";
stockSerializedMessages["Test 6"] = "Test 6 stock message xcvcxvx";
stockSerializedMessages["Test 7"] = "Test 7 stock message fgwqewqdgg";
stockSerializedMessages["Test 8"] = "Test 8 stock message fgfsdfsdgg";

/*
+--------------------------------------------------+
|  define handler                                  |
+--------------------------------------------------+
*/

function setSerializedInputValue(text: string) {
  let e : HTMLTextAreaElement = document.getElementById('TestWasm-SerializedInput') as HTMLTextAreaElement;
  e.value = text;
}

function getSerializedInputValue(): string {
  let e : HTMLTextAreaElement = document.getElementById('TestWasm-SerializedInput') as HTMLTextAreaElement;
  return e.value;
}

function setCppOutputValue(text: string) {
  let e : HTMLTextAreaElement = document.getElementById('TestWasm-CppOutput') as HTMLTextAreaElement;
  e.value = text;
}

function getCppOutputValue(): string {
  let e : HTMLTextAreaElement = document.getElementById('TestWasm-CppOutput') as HTMLTextAreaElement;
  return e.value;
}

function SendFreeTextFromCpp(txt: string):string
{
  setCppOutputValue(getCppOutputValue() + "\n" + txt);
  return "";
}
(<any> window).SendFreeTextFromCpp = SendFreeTextFromCpp;

var referenceMessages = Array<any>();

function testTsCppTs() {
  var r = new TestStoneCodeGen.Message2();
  r.memberEnumMovieType = TestStoneCodeGen.MovieType.RomCom;
  r.memberStringWithDefault = "overriden";
  r.memberMapEnumFloat[TestStoneCodeGen.CrispType.CreamAndChives] = 0.5;
  r.memberString = "reference-messsage2-test1";

  referenceMessages[r.memberString] = r;
  var strMsg2 = r.StoneSerialize();
  let SendMessageToCppForEchoLocal = (<any> window).Module.cwrap('SendMessageToCppForEcho', 'string', ['string']);
  SendMessageToCppForEchoLocal(strMsg2);
}

class MyEchoHandler implements TestStoneCodeGen.IHandler
{
  public HandleMessage2(value:  TestStoneCodeGen.Message2): boolean
  {
    if (value.memberString in referenceMessages) {
      let r = referenceMessages[value.memberString];
      let equals = (value.memberStringWithDefault == r.memberStringWithDefault);
      if (TestStoneCodeGen.CrispType.CreamAndChives in r.memberMapEnumFloat) {
        equals == equals && r.memberMapEnumFloat[TestStoneCodeGen.CrispType.CreamAndChives] == value.memberMapEnumFloat[TestStoneCodeGen.CrispType.CreamAndChives];
      }
      // TODO continue comparison

      if (equals) {
        console.log("objects are equals after round trip");
        return true;
      }
    }
    console.log("problem after round trip");
    return true;
  }
}

function SendMessageFromCpp(txt: string):string
{
  setCppOutputValue(getCppOutputValue() + "\n" + txt);
  TestStoneCodeGen.StoneDispatchToHandler(txt, new MyEchoHandler());
  return "";
}
(<any> window).SendMessageFromCpp = SendMessageFromCpp;



function ButtonClick(buttonName: string) {
  if (buttonName.startsWith('Test ')) {
    setSerializedInputValue(stockSerializedMessages[buttonName]);
  }
  else if (buttonName == "Test-ts-cpp-ts") {
    testTsCppTs();
  }
  else if(buttonName == 'Trigger')
  {
    let serializedInputValue:string = getSerializedInputValue();

    let SendMessageToCppLocal = (<any> window).Module.cwrap('SendMessageToCpp', 'string', ['string']);
    SendMessageToCppLocal(serializedInputValue);
  }
  else if(buttonName == 'Clear')
  {
    setCppOutputValue("");
  }
  else if(buttonName == 'ShowSchema')
  {
    setCppOutputValue(schemaText);
  }
  else
  {
    throw new Error("Internal error!");
  }
}



// this method is called "from the C++ code" when the StoneApplication is updated.
// it can be used to update the UI of the application
function UpdateWebApplicationWithString(statusUpdateMessageString: string) {
  console.log("updating web application (string): ", statusUpdateMessageString);
  let statusUpdateMessage = JSON.parse(statusUpdateMessageString);

  if ("event" in statusUpdateMessage)
  {
    let eventName = statusUpdateMessage["event"];
    if (eventName == "appStatusUpdated")
    {
      //ui.onAppStatusUpdated(statusUpdateMessage["data"]);
    }
  }
}


function UpdateWebApplicationWithSerializedMessage(statusUpdateMessageString: string) {
  console.log("updating web application (serialized message): ", statusUpdateMessageString);
  console.log("<not supported!>");
}
 