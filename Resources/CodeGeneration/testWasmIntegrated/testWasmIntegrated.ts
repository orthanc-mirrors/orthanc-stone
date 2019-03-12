var SendMessageToCpp: Function = null;
export var TestWasmIntegratedModule : any;

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
let schemaText: string = `rootName: testWasmIntegratedCpp

struct B:
  someAs: vector<A>
  someInts: vector<int32>

struct C:
  someBs: vector<B>
  ddd:    vector<string>
  definition: vector<json>

struct A:
  someStrings: vector<string>
  someInts2: vector<int32>
  movies: vector<MovieType>

struct Message1:
  a: int32
  b: string
  c: EnumMonth0
  d: bool

struct Message2:
  toto: string
  tata: vector<Message1>
  tutu: vector<string>
  titi: map<string, string>
  lulu: map<string, Message1>
  movieType: MovieType
  definition: json

enum MovieType:
  - RomCom
  - Horror
  - ScienceFiction
  - Vegetables

enum CrispType:
  - SaltAndPepper
  - CreamAndChives
  - Paprika
  - Barbecue

enum EnumMonth0:
  - January
  - February
  - March
`;

let stockSerializedMessages = new Map<string,string>();
stockSerializedMessages["Test 1"] = `{
  "type" : "testWasmIntegratedCpp.Message2",
  "value" : 
  {
    "lulu" : 
    {
      "54" : 
      {
        "a" : 43,
        "b" : "Sandrine",
        "c" : "March",
        "d" : true
      },
      "55" : 
      {
        "a" : 42,
        "b" : "Benjamin",
        "c" : "January",
        "d" : false
      }
    },
    "tata" : 
    [
      {
        "a" : 42,
        "b" : "Benjamin",
        "c" : "March",
        "d" : false
      },
      {
        "a" : 43,
        "b" : "Sandrine",
        "c" : "January",
        "d" : false
      }
    ],
    "titi" : 
    {
      "44" : "key 44",
      "45" : "key 45"
    },
    "toto" : "Prout zizi",
    "tutu" : 
    [
      "Mercadet",
      "Poisson"
    ],
    "definition":
    {
      "val" : [ "berk", 42 ],
      "zozo" :
      {
        "23": "zloutch",
        "lalala": 42
      }
    }
  }
}`;
stockSerializedMessages["Test 2"] = ` {
  "type" : "testWasmIntegratedCpp.Message1",
  "value" : {
    "a" : -987,
    "b" : "Salomé",
    "c" : 2,
    "d" : true
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


function ButtonClick(buttonName: string) {
  if (buttonName.startsWith('Test ')) {
    setSerializedInputValue(stockSerializedMessages[buttonName]);
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
 