class TestMessage {
    s1: string;
    s2: Array<string>;
    s3: Array<Array<string>>;
    s4: Map<string, number>;
    s5: Map<number, Array<string>>;
    s6: Color;
    s7: boolean;
}

-->

{"s2":["toto","toto2","toto3"],"s4":{"toto":1999,"tatata":1999},"s6":0,"s7":true}

(absent fields weren't specified)


type:B
value:{"someAs":[{...},{},{}]}.........................
Deserialize

jsonValue


