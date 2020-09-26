1. Get Video ID

curl \
  'https://www.googleapis.com/youtube/v3/search?part=snippet&channelId=UCK0xH_L9OBM0CVwC438bMGA&eventType=live&type=video&key=[YOUR_API_KEY]' \
  --header 'Authorization: Bearer [YOUR_ACCESS_TOKEN]' \
  --header 'Accept: application/json' \
  --compressed

2. Get live details

curl \
  'https://www.googleapis.com/youtube/v3/videos?part=liveStreamingDetails&id=wpjUWujUuYU&key=[YOUR_API_KEY]' \
  --header 'Authorization: Bearer [YOUR_ACCESS_TOKEN]' \
  --header 'Accept: application/json' \
  --compressed


3. Get messages

curl \
  'https://www.googleapis.com/youtube/v3/liveChat/messages?liveChatId=Cg0KC3dwalVXdWpVdVlVKicKGFVDSzB4SF9MOU9CTTBDVndDNDM4Yk1HQRILd3BqVVd1alV1WVU&part=snippet&key=[YOUR_API_KEY]' \
  --header 'Authorization: Bearer [YOUR_ACCESS_TOKEN]' \
  --header 'Accept: application/json' \
  --compressed


4. Post messages

curl --request POST \
  'https://www.googleapis.com/youtube/v3/liveChat/messages?part=snippet&key=[YOUR_API_KEY]' \
  --header 'Authorization: Bearer [YOUR_ACCESS_TOKEN]' \
  --header 'Accept: application/json' \
  --header 'Content-Type: application/json' \
  --data '{"snippet":{"liveChatId":"Cg0KC3dwalVXdWpVdVlVKicKGFVDSzB4SF9MOU9CTTBDVndDNDM4Yk1HQRILd3BqVVd1alV1WVU","type":"textMessageEvent","textMessageDetails":{"messageText":"Ehyo!"}}}' \
  --compressed
