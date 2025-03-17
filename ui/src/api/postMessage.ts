// post a message to curl -X POST "http://192.168.4.1/messages"      -H "Content-Type: application/x-www-form-urlencoded"      --data "sender=User1&message=Hello%20ESP32&group=Gaming"
export default function postMessage(sender: string, message: string, group: string) {
    return fetch('http://192.168.4.1/api/messages', {
        method: 'POST',
        headers: new Headers({
            'Content-Type': 'application/x-www-form-urlencoded'
        }),
        body: new URLSearchParams({
            sender: sender, 
            message: message,
            group: group
        })
    });
}