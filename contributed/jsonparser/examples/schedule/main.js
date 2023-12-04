// Copyright 2023 Mark Wharton (mark@jynx.com)
// https://opensource.org/license/mit/

import { JSONParser, Matcher, NodeType, VPT } from "jsonparser";

let fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
let fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
let fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

let keys = ["value", "scheduleItems", "status", "start", "end", "dateTime", "error"];

let vpt = new VPT(keys, new Matcher(
    function (vpt, node) {
        let schedule = this.schedule;
        switch (node.type) {
            case NodeType.field:
                switch (node.text) {
                    case "status":
                        node.up(1, NodeType.object).data.status = node.next.text;
                        break;
                    case "dateTime":
                        node.up(3, NodeType.object).data[node.up(2, NodeType.field).text] = node.next.text;
                        break;
                    case "error":
                        schedule.error = true;
                        break;
                }
                break;
            case NodeType.object:
                if (node.up(2, NodeType.field, "scheduleItems")) {
                    let data = node.data;
                    if (data.status === "busy") {
                        if (schedule.count < 3) {
                            schedule.meetings.push({
                                start: new Date(data.start),
                                end: new Date(data.end)
                            });
                        }
                        schedule.count++;
                    }
                }
                break;
        }
    }, function (vpt) {
        this.schedule = {
            count: 0, error: false, meetings: []
        };
    }
));

let parser = new JSONParser(vpt);

// data example
let data = parser.initialize();
trace(JSON.stringify(data.schedule) + "\n");

// segment example
parser.receive(fragment1);
let start = 0, size = 256;
while (parser.receive(fragment2, start, start + size) > 0) {
    // trace(JSON.stringify(data.schedule) + "\n");
    start += size;
}
Array.from(fragment3).forEach(character => parser.receive(character));

let result = parser.terminate();
if (result === JSONParser.success) {
    data.schedule.meetings.forEach(meeting => {
        trace(`startDateTime: ${meeting.start}, endDateTime: ${meeting.end}\n`);
    });
    trace(JSON.stringify(data.schedule) + "\n");
    trace("success!\n");
} else
    trace(`result: ${result}\n`);

debugger;
