// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/
"use strict";

import { JSONParser, Matcher, NodeType } from "jsonparser";

const fragment1 = '{"@odata.context":"https://graph.microsoft.com/v1.0/$metadata#Collection(microsoft.graph.scheduleInformation)","value":[{"scheduleId":"rm03_boardroom@example.com","availabilityView":"022222200000000000000200","scheduleItems":[{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":';
const fragment2 = '"2023-12-05T13:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T13:25:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Mark Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T14:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T14:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom","isMeeting":true,"isRecurring":false,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T15:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T15:30:00.0000000","timeZone":"E. Australia Standard Time"}},{"isPrivate":false,"status":"busy","subject":"Jethro Wharton","location":"RM-03 \\u2013 Boardroom (AU)","isMeeting":true,"isRecurring":fa';
const fragment3 = 'lse,"isException":false,"isReminderSet":false,"start":{"dateTime":"2023-12-05T23:00:00.0000000","timeZone":"E. Australia Standard Time"},"end":{"dateTime":"2023-12-05T23:01:00.0000000","timeZone":"E. Australia Standard Time"}}],"workingHours":{"daysOfWeek":["monday","tuesday","wednesday","thursday","friday"],"startTime":"08:00:00.0000000","endTime":"17:00:00.0000000","timeZone":{"name":"Pacific Standard Time"}}}]}';

const keys = ["value", "scheduleItems", "status", "start", "end", "dateTime", "error"];

const matcher = new Matcher({
    begin(vpt) {
        vpt.data = {
            error: false,
            schedule: {
                count: 0,
                meetings: []
            }
        };
    },
    match(vpt, node) {
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
                        vpt.data.error = true;
                        break;
                }
                break;
            case NodeType.object:
                if (node.up(2, NodeType.field, "scheduleItems")) {
                    let data = node.data;
                    if (data.status === "busy") {
                        let schedule = vpt.data.schedule;
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
    }
});

const parser = new JSONParser({ keys, matcher });

parser.receive(fragment1);
parser.receive(fragment2);
parser.receive(fragment3);

let result = parser.status;
if (result === JSONParser.success) {
    parser.data.schedule.meetings.forEach(meeting => {
        trace(`startDateTime: ${meeting.start}, endDateTime: ${meeting.end}\n`);
    });
    trace(JSON.stringify(parser.data) + "\n");
    trace("success!\n");
}
else
    trace(`result: ${result}\n`);

parser.close();

debugger;
