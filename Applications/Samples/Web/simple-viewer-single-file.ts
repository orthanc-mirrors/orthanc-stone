import wasmApplicationRunner = require('../../../Platforms/Wasm/wasm-application-runner');

wasmApplicationRunner.InitializeWasmApplication("OrthancStoneSimpleViewerSingleFile", "/orthanc");

function SelectTool(toolName: string) {
    var command = {
        command: "selectTool",
        args: {
            toolName: toolName
        }
    };
    wasmApplicationRunner.SendCommandToStoneApplication(JSON.stringify(command));

}

function PerformAction(commandName: string) {
    var command = {
        command: commandName,
        commandType: "simple",
        args: {}
    };
    wasmApplicationRunner.SendCommandToStoneApplication(JSON.stringify(command));
}

//initializes the buttons
//-----------------------
// install "SelectTool" handlers
document.querySelectorAll("[tool-selector]").forEach((e) => {
    console.log(e);
    (e as HTMLInputElement).addEventListener("click", () => {
        console.log(e);
        SelectTool(e.attributes["tool-selector"].value);
    });
});

// install "PerformAction" handlers
document.querySelectorAll("[action-trigger]").forEach((e) => {
    (e as HTMLInputElement).addEventListener("click", () => {
        PerformAction(e.attributes["action-trigger"].value);
    });
});

// this method is called "from the C++ code" when the StoneApplication is updated.
// it can be used to update the UI of the application
function UpdateWebApplicationWithString(statusUpdateMessage: string) {
  console.log(statusUpdateMessage);
  
  if (statusUpdateMessage.startsWith("series-description=")) {
      document.getElementById("series-description").innerText = statusUpdateMessage.split("=")[1];
  }
}

function UpdateWebApplicationWithSerializedMessage(statusUpdateMessageString: string) {
  console.log("updating web application with serialized message: ", statusUpdateMessageString);
  console.log("<not supported in the simple viewer (single file)!>");
}

// make it available to other js scripts in the application
(<any> window).UpdateWebApplicationWithString = UpdateWebApplicationWithString;

(<any> window).UpdateWebApplicationWithSerializedMessage = UpdateWebApplicationWithSerializedMessage;
