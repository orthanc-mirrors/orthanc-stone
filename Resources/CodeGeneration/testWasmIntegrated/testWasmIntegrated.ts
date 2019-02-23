export var SendMessageToStoneApplication: Function = null;

function SelectTool(toolName: string) {
  var command = {
    command: "selectTool:" + toolName,
    commandType: "generic-no-arg-command",
    args: {
    }                                                                                                                       
  };
  SendMessageToStoneApplication(JSON.stringify(command));
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

// install "SelectTool" handlers
document.querySelectorAll("[tool-selector]").forEach((e) => {
    (e as HTMLButtonElement).addEventListener("click", () => {
    SelectTool(e.attributes["tool-selector"].value);
    });
});

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
