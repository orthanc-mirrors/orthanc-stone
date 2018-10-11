///<reference path='../../../../Platforms/Wasm/wasm-application-runner.ts'/>

InitializeWasmApplication("OrthancStoneSimpleViewer", "/orthanc");

function SelectTool(toolName: string) {
  var command = {
    command: "selectTool:" + toolName,
    commandType: "generic-no-arg-command",
    args: {
    }                                                                                                                       
  };
  SendMessageToStoneApplication(JSON.stringify(command));

}

function PerformAction(actionName: string) {
  var command = {
    command: "action:" + actionName,
    commandType: "generic-no-arg-command",
    args: {
    }
  };
  SendMessageToStoneApplication(JSON.stringify(command));
}

class SimpleViewerUI {

  private _labelPatientId: HTMLSpanElement;
  private _labelStudyDescription: HTMLSpanElement;

  public constructor() {
    // install "SelectTool" handlers
    document.querySelectorAll("[tool-selector]").forEach((e) => {
      console.log(e);
      (e as HTMLButtonElement).addEventListener("click", () => {
        console.log(e);
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
function UpdateWebApplication(statusUpdateMessageString: string) {
  console.log("updating web application: ", statusUpdateMessageString);
  let statusUpdateMessage = JSON.parse(statusUpdateMessageString);

  if ("event" in statusUpdateMessage) {
    let eventName = statusUpdateMessage["event"];
    if (eventName == "appStatusUpdated") {
      ui.onAppStatusUpdated(statusUpdateMessage["data"]);
    }
  }
}
