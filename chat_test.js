// test-chatbot-matching.js
const faqData = require('./faqData.json');

// Fuzzy search function (same as in your component)
function findBestMatch(userInput, language = 'en') {
    const input = userInput.toLowerCase().trim();
    let bestMatch = null;
    let highestScore = 0;
    let matchedCategory = '';

    // Search through all FAQ categories
    Object.entries(faqData.faq).forEach(([categoryName, category]) => {
        category.forEach(item => {
            const question = (item.question[language] || item.question.en).toLowerCase();
            const answer = item.answer[language] || item.answer.en;

            // Calculate similarity score
            let score = 0;
            
            // Exact match bonus
            if (question.includes(input) || input.includes(question)) {
                score += 100;
            }

            // Word matching
            const inputWords = input.split(/\s+/);
            const questionWords = question.split(/\s+/);
            
            inputWords.forEach(word => {
                if (word.length > 2) {
                    questionWords.forEach(qWord => {
                        if (qWord.includes(word) || word.includes(qWord)) {
                            score += 10;
                        }
                    });
                }
            });

            // Keyword matching for common topics
            const keywords = {
                wifi: ['wifi', 'connect', 'connection', 'network', 'সংযুক্ত', 'ওয়াইফাই', 'WiFi', '接続'],
                sensor: ['sensor', 'temperature', 'humidity', 'co2', 'সেন্সর', 'তাপমাত্রা', 'センサー', '温度'],
                dashboard: ['dashboard', 'use', 'monitor', 'ড্যাশবোর্ড', 'ব্যবহার', 'ダッシュボード', '使い方'],
                fermentation: ['ferment', 'brew', 'sake', 'ফার্মেন্টেশন', 'সাকে', '発酵', '酒'],
                esp32: ['esp32', 'esp', 'microcontroller', 'মাইক্রোকন্ট্রোলার', 'マイコン'],
                troubleshoot: ['problem', 'error', 'not working', 'সমস্যা', 'エラー', '問題'],
                mqtt: ['mqtt', 'topic', 'data', 'টপিক', 'ডেটা', 'トピック', 'データ'],
                actuator: ['fan', 'pump', 'control', 'ফ্যান', 'পাম্প', 'ファン', 'ポンプ']
            };

            Object.entries(keywords).forEach(([topic, words]) => {
                words.forEach(keyword => {
                    if (input.includes(keyword.toLowerCase())) {
                        if (question.includes(keyword.toLowerCase())) {
                            score += 20;
                        }
                    }
                });
            });

            if (score > highestScore) {
                highestScore = score;
                bestMatch = {
                    question: item.question[language] || item.question.en,
                    answer: answer,
                    score: score
                };
                matchedCategory = categoryName;
            }
        });
    });

    return highestScore > 15 ? { ...bestMatch, category: matchedCategory } : null;
}

// Color codes for terminal output
const colors = {
    reset: '\x1b[0m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    cyan: '\x1b[36m',
    magenta: '\x1b[35m'
};

function testMatching() {
    console.log(colors.cyan + '\n========================================');
    console.log('       ChatBot FAQ Matching Test');
    console.log('========================================' + colors.reset);

    const testQueries = [
        // English queries
        { query: "wifi connection", expectedTopic: "WiFi/ESP32", language: "en" },
        { query: "How do I connect ESP32?", expectedTopic: "WiFi/ESP32", language: "en" },
        { query: "sensors", expectedTopic: "Sensors", language: "en" },
        { query: "What sensors are used?", expectedTopic: "Sensors", language: "en" },
        { query: "dashboard use", expectedTopic: "Dashboard", language: "en" },
        { query: "How to use dashboard?", expectedTopic: "Dashboard", language: "en" },
        { query: "co2 level", expectedTopic: "CO2/Fermentation", language: "en" },
        { query: "What does CO2 mean?", expectedTopic: "CO2/Fermentation", language: "en" },
        { query: "temperature monitoring", expectedTopic: "Temperature/Sensors", language: "en" },
        { query: "optimal conditions", expectedTopic: "Fermentation", language: "en" },
        { query: "mqtt topics", expectedTopic: "MQTT", language: "en" },
        { query: "fan control", expectedTopic: "Actuators", language: "en" },
        { query: "ESP32 not connecting", expectedTopic: "Troubleshooting", language: "en" },
        { query: "data not showing", expectedTopic: "Troubleshooting", language: "en" },
        
        // Bangla queries
        { query: "আমি কীভাবে ড্যাশবোর্ড ব্যবহার করব", expectedTopic: "Dashboard", language: "bn" },
        { query: "সেন্সর", expectedTopic: "Sensors", language: "bn" },
        
        // Japanese queries
        { query: "ダッシュボードの使い方", expectedTopic: "Dashboard", language: "ja" },
        { query: "センサー", expectedTopic: "Sensors", language: "ja" },
        
        // Edge cases
        { query: "random gibberish xyz123", expectedTopic: "NO MATCH", language: "en" },
        { query: "hello", expectedTopic: "NO MATCH", language: "en" },
    ];

    let passed = 0;
    let failed = 0;

    testQueries.forEach(({ query, expectedTopic, language }, index) => {
        console.log(colors.blue + `\n[Test ${index + 1}/${testQueries.length}]` + colors.reset);
        console.log(`Query: "${query}" (${language})`);
        console.log(`Expected: ${expectedTopic}`);
        
        const match = findBestMatch(query, language);
        
        if (match) {
            console.log(colors.green + `✓ Match Found!` + colors.reset);
            console.log(`  Category: ${match.category}`);
            console.log(`  Score: ${match.score}`);
            console.log(`  Question: ${match.question.substring(0, 80)}...`);
            console.log(`  Answer Preview: ${match.answer.substring(0, 100)}...`);
            passed++;
        } else {
            if (expectedTopic === "NO MATCH") {
                console.log(colors.green + `✓ Correctly returned no match` + colors.reset);
                passed++;
            } else {
                console.log(colors.red + `✗ No match found (expected: ${expectedTopic})` + colors.reset);
                failed++;
            }
        }
    });

    // Summary
    console.log(colors.cyan + '\n========================================');
    console.log('              Test Summary');
    console.log('========================================' + colors.reset);
    console.log(colors.green + `Passed: ${passed}` + colors.reset);
    console.log(colors.red + `Failed: ${failed}` + colors.reset);
    console.log(colors.yellow + `Total: ${testQueries.length}` + colors.reset);
    console.log(colors.cyan + `Success Rate: ${((passed / testQueries.length) * 100).toFixed(1)}%` + colors.reset);
    console.log(colors.cyan + '========================================\n' + colors.reset);
}

// Run the test
testMatching();

// Additional: Test specific queries interactively
console.log(colors.magenta + '\n💡 You can test specific queries by modifying the testQueries array above.' + colors.reset);
console.log(colors.magenta + '💡 Or run: node test.js "your custom query here"\n' + colors.reset);

// Handle command line arguments for custom testing
if (process.argv.length > 2) {
    const customQuery = process.argv.slice(2).join(' ');
    console.log(colors.yellow + `\n🔍 Testing custom query: "${customQuery}"` + colors.reset);
    
    const result = findBestMatch(customQuery, 'en');
    if (result) {
        console.log(colors.green + '\n✓ Match found!' + colors.reset);
        console.log(`Category: ${result.category}`);
        console.log(`Score: ${result.score}`);
        console.log(`Question: ${result.question}`);
        console.log(`\nAnswer:\n${result.answer}`);
    } else {
        console.log(colors.red + '\n✗ No match found' + colors.reset);
    }
}