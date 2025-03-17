import { Message } from ".";
export default async function getMessages(channel: string): Promise<Message[]> {
    const result = await fetch(`http://192.168.4.1/api/messages?group=${channel}`, {
        method: 'GET'
    });
    const data = await result.json();
    return data as Message[];
}