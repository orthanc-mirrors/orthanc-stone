export enum LogSource {
  Cpp,
  Typescript
}

export class StandardConsoleLogger {
  public showSource: boolean = true;

  public debug(...args: any[]): void {
    this._debug(LogSource.Typescript, ...args);
  }

  public debugFromCpp(...args: any[]): void {
    this._debug(LogSource.Cpp, ...args);
  }

  public info(...args: any[]): void {
    this._info(LogSource.Typescript, ...args);
  }

  public infoFromCpp(message: string): void {
    this._info(LogSource.Cpp, message);
  }

  public warning(...args: any[]): void {
    this._warning(LogSource.Typescript, ...args);
  }

  public warningFromCpp(message: string): void {
    this._warning(LogSource.Cpp, message);
  }

  public error(...args: any[]): void {
    this._error(LogSource.Typescript, ...args);
  }

  public errorFromCpp(message: string): void {
    this._error(LogSource.Cpp, message);
  }

  public _debug(source: LogSource, ...args: any[]): void {
    if ((<any> window).IsTraceLevelEnabled)
    {
      if ((<any> window).IsTraceLevelEnabled())
      {
        var output = this.getOutput(source, args);
        console.debug(...output);
      }
    }
  }

  private _info(source: LogSource, ...args: any[]): void {
    if ((<any> window).IsInfoLevelEnabled)
    {
      if ((<any> window).IsInfoLevelEnabled())
      {
        var output = this.getOutput(source, args);
        console.info(...output);
      }
    }
  }

  public _warning(source: LogSource, ...args: any[]): void {
    var output = this.getOutput(source, args);
    console.warn(...output);
  }

  public _error(source: LogSource, ...args: any[]): void {
    var output = this.getOutput(source, args);
    console.error(...output);
  }


  private getOutput(source: LogSource, args: any[]): any[] {
    var prefix = this.getPrefix();
    var prefixAndSource = [];

    if (prefix != null) {
      prefixAndSource = [prefix];
    } 

    if (this.showSource) {
      if (source == LogSource.Typescript) {
        prefixAndSource = [...prefixAndSource, "TS "];
      } else if (source == LogSource.Cpp) {
        prefixAndSource = [...prefixAndSource, "C++"];
      }
    }

    if (prefixAndSource.length > 0) {
      prefixAndSource = [...prefixAndSource, "|"];
    }

    return [...prefixAndSource, ...args];
  }

  protected getPrefix(): string {
    return null;
  }
}

export class TimeConsoleLogger extends StandardConsoleLogger {
  protected getPrefix(): string {
    let now = new Date();
    let timeString = now.getHours().toString().padStart(2, "0") + ":" + now.getMinutes().toString().padStart(2, "0") + ":" + now.getSeconds().toString().padStart(2, "0") + "." + now.getMilliseconds().toString().padStart(3, "0");
    return timeString;
  }
}

export var defaultLogger: StandardConsoleLogger = new TimeConsoleLogger();

