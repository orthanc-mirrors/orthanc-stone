export var SendMessageToStoneApplication: Function = null;

// install handlers
document.querySelectorAll(".TestWasm-button").forEach((e) => {
  (e as HTMLButtonElement).addEventListener("click", () => {
    ButtonClick(e.attributes["tool-selector"].value);
  });
});

let stockSerializedMessages = new Map<string,string>();

stockSerializedMessages["Test 1"] = "Test 1 stock message fgdgg";
stockSerializedMessages["Test 2"] = "Test 2 stock message bccbbbbbb";
stockSerializedMessages["Test 3"] = "Test 3 stock message sdfsfsdfsdf";
stockSerializedMessages["Test 4"] = "Test 4 stock message 355345345";
stockSerializedMessages["Test 5"] = "Test 5 stock message 34535";
stockSerializedMessages["Test 6"] = "Test 6 stock message xcvcxvx";
stockSerializedMessages["Test 7"] = "Test 7 stock message fgwqewqdgg";
stockSerializedMessages["Test 8"] = "Test 8 stock message fgfsdfsdgg";

function ButtonClick(buttonName: string) {
  if (buttonName.startsWith('Test ')) {
    let e : HTMLTextAreaElement = document.getElementById('TestWasm-SerializedInput') as HTMLTextAreaElement;
    e.value = stockSerializedMessages[buttonName];
  }
  else if(buttonName == 'Trigger')
  {
    console.error('Not implemented!')
  }
}

(<any> window).StoneFrameworkModule = {
  preRun: [ 
    function() {
    console.log('Loading the Stone Framework using WebAssembly');
    }
  ],
  postRun: [ 
    function()  {
    // This function is called by ".js" wrapper once the ".wasm"
    // WebAssembly module has been loaded and compiled by the
    // browser
    console.log('WebAssembly is ready');
    SendMessageToStoneApplication = (<any> window).StoneFrameworkModule.cwrap('SendMessageToStoneApplication', 'string', ['string']);
    }
  ],
  print: function(text : string) {
    console.log(text);
  },
  printErr: function(text : string) {
    console.error(text);
  },
  totalDependencies: 0
};



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
