///<reference path='../../../Platforms/Wasm/wasm-application-runner.ts'/>

InitializeWasmApplication("OrthancStoneSimpleViewer", "/orthanc");

function SelectTool(toolName: string) {
    SendMessageToStoneApplication("select-tool:" + toolName);
}

function PerformAction(actionName: string) {
    SendMessageToStoneApplication("perform-action:" + actionName);
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
