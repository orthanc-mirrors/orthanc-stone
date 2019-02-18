class Greeter {
    greeting: string;
    constructor(message: string) {
        this.greeting = message;
    }
    greet() {
        return "Hello, " + this.greeting;
    }
}
enum Color { Red, Green, Blue };

class TestMessage {
    s1: string;
    s2: Array<string>;
    s3: Array<Array<string>>;
    s4: Map<string, number>;
    s5: Map<number, Array<string>>;
    s6: Color;
    s7: boolean;
}

let tm = new TestMessage();
tm.s2 = new Array<string>()
tm.s2.push("toto");
tm.s2.push("toto2");
tm.s2.push("toto3");
tm.s4 = new Map<string, number>();
tm.s4["toto"] = 42;
tm.s4["toto"] = 1999;
tm.s4["tatata"] = 1999;
tm.s6 = Color.Red;
tm.s7 = true

let txt = JSON.stringify(tm)
console.log(txt);

let greeter = new Greeter("world");

