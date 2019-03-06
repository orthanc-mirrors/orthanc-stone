import wasmApplicationRunner = require('../../../../Platforms/Wasm/wasm-application-runner');

wasmApplicationRunner.InitializeWasmApplication("OrthancStoneSimpleViewer", "/orthanc");

function SelectTool(toolName: string) {
  var command = {
    command: "selectTool:" + toolName,
    commandType: "generic-no-arg-command",
    args: {
    }                                                                                                                       
  };
  wasmApplicationRunner.SendMessageToStoneApplication(JSON.stringify(command));
}

function PerformAction(actionName: string) {
  var command = {
    command: "action:" + actionName,
    commandType: "generic-no-arg-command",
    args: {
    }
  };
  wasmApplicationRunner.SendMessageToStoneApplication(JSON.stringify(command));
}

class SimpleViewerUI {

  private _labelPatientId: HTMLSpanElement;
  private _labelStudyDescription: HTMLSpanElement;

  public constructor() {
    // install "SelectTool" handlers
    document.querySelectorAll("[tool-selector]").forEach((e) => {
      (e as HTMLButtonElement).addEventListener("click", () => {
        SelectTool(e.attributes["tool-selector"].value);
      });
    });

    // install "PerformAction" handlers
    document.querySelectorAll("[action-trigger]").forEach((e) => {
      (e as HTMLButtonElement).addEventListener("click", () => {
        PerformAction(e.attributes["action-trigger"].value);
      });
    });

    // connect all ui elements to members
    this._labelPatientId = document.getElementById("label-patient-id") as HTMLSpanElement;
    this._labelStudyDescription = document.getElementById("label-study-description") as HTMLSpanElement;
  }

  public onAppStatusUpdated(status: any) {
    this._labelPatientId.innerText = status["patientId"];
    this._labelStudyDescription.innerText = status["studyDescription"];
    // this.highlighThumbnail(status["currentInstanceIdInMainViewport"]);
  }

}

var ui = new SimpleViewerUI();

// this method is called "from the C++ code" when the StoneApplication is updated.
// it can be used to update the UI of the application
function UpdateWebApplicationWithString(statusUpdateMessageString: string) {
  console.log("updating web application with string: ", statusUpdateMessageString);
  let statusUpdateMessage = JSON.parse(statusUpdateMessageString);

  if ("event" in statusUpdateMessage) {
    let eventName = statusUpdateMessage["event"];
    if (eventName == "appStatusUpdated") {
      ui.onAppStatusUpdated(statusUpdateMessage["data"]);
    }
  }
}

function UpdateWebApplicationWithSerializedMessage(statusUpdateMessageString: string) {
  console.log("updating web application with serialized message: ", statusUpdateMessageString);
  console.log("<not supported in the simple viewer!>");
}

// make it available to other js scripts in the application
(<any> window).UpdateWebApplicationWithString = UpdateWebApplicationWithString;
(<any> window).UpdateWebApplicationWithSerializedMessage = UpdateWebApplicationWithSerializedMessage;
