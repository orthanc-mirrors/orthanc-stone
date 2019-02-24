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
        "c" : 2,
        "d" : true
      },
      "55" : 
      {
        "a" : 42,
        "b" : "Benjamin",
        "c" : 0,
        "d" : false
      }
    },
    "tata" : 
    [
      {
        "a" : 42,
        "b" : "Benjamin",
        "c" : 0,
        "d" : false
      },
      {
        "a" : 43,
        "b" : "Sandrine",
        "c" : 2,
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
    ]
  }
}`;
stockSerializedMessages["Test 2"] = "Test 2 stock message bccbbbbbb";
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
  setCppOutputValue(getCppOutputValue() + txt);
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
}



// this method is called "from the C++ code" when the StoneApplication is updated.
// it can be used to update the UI of the application
function UpdateWebApplication(statusUpdateMessageString: string) {
  console.log("updating web application: ", statusUpdateMessageString);
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
