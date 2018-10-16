///<reference path='../../../Platforms/Wasm/wasm-application-runner.ts'/>

InitializeWasmApplication("OrthancStoneSimpleViewerSingleFile", "/orthanc");

function SelectTool(toolName: string) {
    var command = {
        command: "selectTool",
        args: {
            toolName: toolName
        }
    };
    SendMessageToStoneApplication(JSON.stringify(command));

}

function PerformAction(commandName: string) {
    var command = {
        command: commandName,
        commandType: "simple",
        args: {}
    };
    SendMessageToStoneApplication(JSON.stringify(command));
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
function UpdateWebApplication(statusUpdateMessage: string) {
  console.log(statusUpdateMessage);
  
  if (statusUpdateMessage.startsWith("series-description=")) {
      document.getElementById("series-description").innerText = statusUpdateMessage.split("=")[1];
  }
}
