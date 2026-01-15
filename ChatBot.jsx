
'use client';
import React, { useState, useRef, useEffect } from "react";
import { FiSend, FiMessageSquare } from "react-icons/fi";
import { BsRobot } from "react-icons/bs";
import { IoPersonCircleOutline } from "react-icons/io5";
import ReactMarkdown from 'react-markdown';
import { usePathname } from 'next/navigation';
import { useTranslation } from '../../app/i18n/client.js';
import faqData from './faqData.json'; // Import the FAQ JSON file

const ChatBot = () => {
    const [messages, setMessages] = useState([]);
    const [input, setInput] = useState("");
    const [isLoading, setIsLoading] = useState(false);
    const [showSuggestions, setShowSuggestions] = useState(true);
    const chatBoxRef = useRef(null);

    const pathname = usePathname();
    const lng = pathname.split("/")[1] || 'en';
    const { t } = useTranslation(lng, "chatbot");

    // Suggested questions by language
    const suggestedQuestions = {
        en: [
     
            "How do I use the dashboard?",
            "What are the optimal fermentation conditions?",
            "How do I interpret CO₂ levels?",
           
            "What are the MQTT topics?",
          
        ],
        bn: [
       
            "আমি কীভাবে ড্যাশবোর্ড ব্যবহার করব?",
            "সর্বোত্তম ফার্মেন্টেশন অবস্থা কী?",
            "আমি কীভাবে CO₂ স্তর ব্যাখ্যা করব?",
           
            "সিস্টেমে কোন MQTT টপিক ব্যবহার করা হয়?",
           
        ],
        ja: [
     
            "ダッシュボードの使い方は？",
            "最適な発酵条件は何ですか？",
            "CO₂レベルをどのように解釈しますか？",
         
            "システムで使用されているMQTTトピックは何ですか？",
           
        ]
    };

    // Auto-scroll to bottom
    useEffect(() => {
        if (chatBoxRef.current) {
            chatBoxRef.current.scrollTop = chatBoxRef.current.scrollHeight;
        }
    }, [messages]);

    // Welcome message on first load
    useEffect(() => {
        const welcomeMessages = {
            en: "Hello! I'm Sake-IoT, your sake brewing assistant. I can help you with system setup, sensor information, fermentation monitoring, and troubleshooting. How may I assist you today?",
            bn: "হ্যালো! আমি Sake-IoT, আপনার সাকে ব্রুইং সহায়ক। আমি আপনাকে সিস্টেম সেটআপ, সেন্সর তথ্য, ফার্মেন্টেশন মনিটরিং এবং সমস্যা সমাধানে সাহায্য করতে পারি। আজ আমি আপনাকে কীভাবে সাহায্য করতে পারি?",
            ja: "こんにちは！私はSake-IoT、あなたの酒造りアシスタントです。システムセットアップ、センサー情報、発酵監視、トラブルシューティングをお手伝いできます。今日はどのようにお手伝いしましょうか？"
        };

        if (messages.length === 0) {
            setMessages([{
                sender: "bot",
                text: welcomeMessages[lng] || welcomeMessages.en,
                isMarkdown: false
            }]);
        }
    }, []);

    // Fuzzy search function for finding best matching FAQ
    const findBestMatch = (userInput, language) => {
        const input = userInput.toLowerCase().trim();
        let bestMatch = null;
        let highestScore = 0;

        // Search through all FAQ categories
        Object.values(faqData.faq).forEach(category => {
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
                        answer: answer
                    };
                }
            });
        });

        return highestScore > 15 ? bestMatch : null;
    };

    // Handle suggested question click
    const handleSuggestionClick = (question) => {
        setInput(question);
        setShowSuggestions(false);
    };

    // Handle sending message
    const sendMessage = async () => {
        if (!input.trim()) return;

        setShowSuggestions(false);
        const userMsg = { sender: "user", text: input };
        setMessages((prev) => [...prev, userMsg]);
        const userInput = input;
        setInput("");
        setIsLoading(true);

        // Simulate processing delay
        await new Promise(resolve => setTimeout(resolve, 500));

        try {
            const match = findBestMatch(userInput, lng);

            if (match) {
                // Found a matching FAQ
                setMessages((prev) => [
                    ...prev,
                    { 
                        sender: "bot", 
                        text: match.answer, 
                        isMarkdown: true 
                    }
                ]);
            } else {
                // No match found - provide helpful response
                const noMatchResponses = {
                    en: "I'm sorry, I couldn't find a specific answer to your question. Here are some topics I can help you with:\n\n- **System Connection**: WiFi setup, ESP32 connection, software installation\n- **System Usage**: Dashboard features, MQTT topics, data monitoring\n- **Brewing Knowledge**: Optimal conditions, CO₂ levels, sugar/°Brix interpretation\n- **Sensors**: Sensor types, temperature monitoring, AI camera\n- **Troubleshooting**: Connection issues, data problems, fermentation status\n- **Actuators**: Fan and pump controls\n- **Technical**: System technology, security, benefits\n\nPlease try asking your question differently, or ask about one of these topics!",
                    bn: "দুঃখিত, আমি আপনার প্রশ্নের নির্দিষ্ট উত্তর খুঁজে পাইনি। এখানে কিছু বিষয় রয়েছে যাতে আমি আপনাকে সাহায্য করতে পারি:\n\n- **সিস্টেম সংযোগ**: WiFi সেটআপ, ESP32 সংযোগ, সফটওয়্যার ইনস্টলেশন\n- **সিস্টেম ব্যবহার**: ড্যাশবোর্ড বৈশিষ্ট্য, MQTT টপিক, ডেটা মনিটরিং\n- **ব্রুইং জ্ঞান**: সর্বোত্তম অবস্থা, CO₂ স্তর, চিনি/°Brix ব্যাখ্যা\n- **সেন্সর**: সেন্সরের ধরন, তাপমাত্রা মনিটরিং, AI ক্যামেরা\n- **সমস্যা সমাধান**: সংযোগ সমস্যা, ডেটা সমস্যা, ফার্মেন্টেশন স্ট্যাটাস\n- **অ্যাকচুয়েটর**: ফ্যান এবং পাম্প নিয়ন্ত্রণ\n- **প্রযুক্তিগত**: সিস্টেম প্রযুক্তি, নিরাপত্তা, সুবিধা\n\nঅনুগ্রহ করে আপনার প্রশ্ন ভিন্নভাবে জিজ্ঞাসা করার চেষ্টা করুন, অথবা এই বিষয়গুলির একটি সম্পর্কে জিজ্ঞাসা করুন!",
                    ja: "申し訳ございませんが、質問に対する具体的な回答が見つかりませんでした。以下のトピックについてお手伝いできます:\n\n- **システム接続**: WiFiセットアップ、ESP32接続、ソフトウェアインストール\n- **システム使用**: ダッシュボード機能、MQTTトピック、データ監視\n- **醸造知識**: 最適条件、CO₂レベル、糖度/°Brix解釈\n- **センサー**: センサータイプ、温度監視、AIカメラ\n- **トラブルシューティング**: 接続問題、データ問題、発酵状態\n- **アクチュエーター**: ファンとポンプの制御\n- **技術**: システム技術、セキュリティ、利点\n\n質問を言い換えてみるか、これらのトピックのいずれかについて質問してください！"
                };

                setMessages((prev) => [
                    ...prev,
                    { 
                        sender: "bot", 
                        text: noMatchResponses[lng] || noMatchResponses.en, 
                        isMarkdown: true 
                    }
                ]);
            }

        } catch (err) {
            console.error('Chatbot error:', err);
            const errorMessages = {
                en: "I encountered an error. Please try asking your question again.",
                bn: "আমি একটি ত্রুটির সম্মুখীন হয়েছি। অনুগ্রহ করে আপনার প্রশ্ন আবার জিজ্ঞাসা করুন।",
                ja: "エラーが発生しました。もう一度質問してください。"
            };
            setMessages((prev) => [
                ...prev,
                { sender: "bot", text: errorMessages[lng] || errorMessages.en, isMarkdown: false },
            ]);
        } finally {
            setIsLoading(false);
        }
    };

    const handleKeyPress = (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            sendMessage();
        }
    };

    const startNewSession = () => {
        const welcomeMessages = {
            en: "New chat started! How can I help you with sake brewing today?",
            bn: "নতুন চ্যাট শুরু হয়েছে! আজ আমি কীভাবে সাকে ব্রুইংয়ে আপনাকে সাহায্য করতে পারি?",
            ja: "新しいチャットが開始されました！今日の酒造りについてどのようにお手伝いしましょうか？"
        };
        
        setMessages([{
            sender: "bot",
            text: welcomeMessages[lng] || welcomeMessages.en,
            isMarkdown: false
        }]);
        setShowSuggestions(true);
    };

    const questionLabels = {
        en: "Try asking:",
        bn: "এই প্রশ্নগুলি জিজ্ঞাসা করুন:",
        ja: "質問してみてください:"
    };

    return (
        <div className="w-full max-w-2xl mx-auto mt-8 rounded-2xl shadow-2xl overflow-hidden bg-gradient-to-br from-slate-50 to-slate-100 dark:from-gray-800 dark:to-gray-900 border border-slate-200 dark:border-gray-700 transition-colors duration-300">
            {/* Header */}
            <div className="bg-gradient-to-r from-blue-600 to-blue-700 dark:from-blue-700 dark:to-blue-800 px-6 py-4 flex items-center gap-3 shadow-lg">
                <div className="bg-white/20 backdrop-blur-sm p-2 rounded-xl">
                    <BsRobot className="text-white text-2xl" />
                </div>
                <div className="flex-1">
                    <h2 className="text-white text-xl font-bold tracking-tight">Sake-IoT AI</h2>
                    <p className="text-blue-100 text-sm">{t("Sake Brewing Assistant")}</p>
                </div>
                <button
                    onClick={startNewSession}
                    className="text-white/80 hover:text-white text-xs underline transition-colors"
                >
                    {t("New Chat")}
                </button>
                <div>
                    <span className="flex items-center gap-2 text-white/90 text-xs">
                        <span className="w-2 h-2 bg-green-400 rounded-full animate-pulse"></span>
                        {t("Online")}
                    </span>
                </div>
            </div>

            {/* Chat Messages */}
            <div
                ref={chatBoxRef}
                className="h-[450px] overflow-y-auto px-6 py-4 space-y-4 bg-gradient-to-b from-slate-50 to-white dark:from-gray-800 dark:to-gray-900 scrollbar-thin scrollbar-thumb-slate-300 dark:scrollbar-thumb-gray-600 scrollbar-track-transparent"
            >
                {messages.map((msg, i) => (
                    <div
                        key={i}
                        className={`flex items-start gap-3 ${msg.sender === "user" ? "flex-row-reverse" : "flex-row"
                            }`}
                    >
                        {/* Avatar */}
                        <div
                            className={`flex-shrink-0 w-10 h-10 rounded-full flex items-center justify-center shadow-md ${msg.sender === "user"
                                ? "bg-gradient-to-br from-blue-500 to-blue-600"
                                : "bg-gradient-to-br from-green-500 to-emerald-600"
                                }`}
                        >
                            {msg.sender === "user" ? (
                                <IoPersonCircleOutline className="text-white text-2xl" />
                            ) : (
                                <BsRobot className="text-white text-xl" />
                            )}
                        </div>

                        {/* Message Bubble */}
                        <div
                            className={`max-w-[75%] px-4 py-3 rounded-2xl shadow-md transition-all duration-300 hover:shadow-lg ${msg.sender === "user"
                                ? "bg-gradient-to-br from-blue-500 to-blue-600 text-white rounded-tr-sm"
                                : "bg-white dark:bg-gray-700 text-slate-800 dark:text-gray-200 border border-slate-200 dark:border-gray-600 rounded-tl-sm"
                                }`}
                        >
                            {msg.isMarkdown && msg.sender === "bot" ? (
                                <div className="markdown-content text-sm leading-relaxed">
                                    <ReactMarkdown
                                        components={{
                                            h1: ({ node, ...props }) => <h1 className="text-xl font-bold text-slate-800 dark:text-gray-200 mb-3 mt-4 first:mt-0" {...props} />,
                                            h2: ({ node, ...props }) => <h2 className="text-lg font-bold text-slate-800 dark:text-gray-200 mb-2 mt-4 first:mt-0" {...props} />,
                                            h3: ({ node, ...props }) => <h3 className="text-base font-bold text-slate-800 dark:text-gray-200 mb-2 mt-3 first:mt-0" {...props} />,
                                            p: ({ node, ...props }) => <p className="text-slate-700 dark:text-gray-300 mb-2 leading-relaxed" {...props} />,
                                            ul: ({ node, ...props }) => <ul className="list-disc pl-5 my-2 space-y-1" {...props} />,
                                            ol: ({ node, ...props }) => <ol className="list-decimal pl-5 my-2 space-y-1" {...props} />,
                                            li: ({ node, ...props }) => <li className="text-slate-700 dark:text-gray-300 leading-relaxed" {...props} />,
                                            strong: ({ node, ...props }) => <strong className="font-semibold text-slate-900 dark:text-gray-100" {...props} />,
                                            em: ({ node, ...props }) => <em className="italic text-slate-700 dark:text-gray-300" {...props} />,
                                            code: ({ node, inline, ...props }) =>
                                                inline ? (
                                                    <code className="bg-slate-100 dark:bg-gray-600 px-1.5 py-0.5 rounded text-xs font-mono text-slate-800 dark:text-gray-200" {...props} />
                                                ) : (
                                                    <code className="block bg-slate-100 dark:bg-gray-600 p-3 rounded-lg text-xs font-mono text-slate-800 dark:text-gray-200 my-2 overflow-x-auto" {...props} />
                                                ),
                                            blockquote: ({ node, ...props }) => <blockquote className="border-l-4 border-slate-300 dark:border-gray-500 pl-4 italic text-slate-600 dark:text-gray-400 my-2" {...props} />,
                                            a: ({ node, ...props }) => <a className="text-blue-600 dark:text-blue-400 hover:text-blue-700 dark:hover:text-blue-300 underline" {...props} />,
                                        }}
                                    >
                                        {msg.text}
                                    </ReactMarkdown>
                                </div>
                            ) : (
                                <p className="text-sm leading-relaxed whitespace-pre-wrap break-words">
                                    {msg.text}
                                </p>
                            )}
                        </div>
                    </div>
                ))}

                {/* Suggested Questions */}
                {showSuggestions && messages.length > 0 && (
                    <div className="mt-6">
                        <p className="text-sm font-semibold text-slate-600 dark:text-gray-400 mb-3 px-2">
                            {questionLabels[lng] || questionLabels.en}
                        </p>
                        <div className="grid grid-cols-1 gap-2">
                            {(suggestedQuestions[lng] || suggestedQuestions.en).slice(0, 6).map((question, idx) => (
                                <button
                                    key={idx}
                                    onClick={() => handleSuggestionClick(question)}
                                    className="text-left px-4 py-3 rounded-xl bg-white dark:bg-gray-700 border-2 border-slate-200 dark:border-gray-600 hover:border-blue-400 dark:hover:border-blue-500 hover:shadow-md transition-all duration-200 text-sm text-slate-700 dark:text-gray-300 hover:text-blue-600 dark:hover:text-blue-400"
                                >
                                    💡 {question}
                                </button>
                            ))}
                        </div>
                    </div>
                )}

                {/* Typing Indicator */}
                {isLoading && (
                    <div className="flex items-start gap-3">
                        <div className="flex-shrink-0 w-10 h-10 rounded-full bg-gradient-to-br from-green-500 to-emerald-600 flex items-center justify-center shadow-md">
                            <BsRobot className="text-white text-xl" />
                        </div>
                        <div className="bg-white dark:bg-gray-700 px-5 py-3 rounded-2xl rounded-tl-sm border border-slate-200 dark:border-gray-600 shadow-md">
                            <div className="flex gap-1.5">
                                <span className="w-2 h-2 bg-slate-400 dark:bg-gray-500 rounded-full animate-bounce" style={{ animationDelay: "0ms" }}></span>
                                <span className="w-2 h-2 bg-slate-400 dark:bg-gray-500 rounded-full animate-bounce" style={{ animationDelay: "150ms" }}></span>
                                <span className="w-2 h-2 bg-slate-400 dark:bg-gray-500 rounded-full animate-bounce" style={{ animationDelay: "300ms" }}></span>
                            </div>
                        </div>
                    </div>
                )}
            </div>

            {/* Input Area */}
            <div className="bg-white dark:bg-gray-800 border-t border-slate-200 dark:border-gray-700 px-6 py-4">
                <div className="flex items-center gap-3">
                    <div className="flex-1 relative">
                        <textarea
                            ref={(el) => {
                                if (el) {
                                    el.style.height = 'auto';
                                    el.style.height = Math.min(el.scrollHeight, 120) + 'px';
                                }
                            }}
                            className="w-full px-4 py-3 rounded-xl border-2 border-slate-200 dark:border-gray-600 focus:border-blue-500 dark:focus:border-blue-400 focus:ring-4 focus:ring-blue-100 dark:focus:ring-blue-900/30 outline-none resize-none transition-all duration-200 text-slate-800 dark:text-gray-200 placeholder-slate-400 dark:placeholder-gray-500 bg-white dark:bg-gray-700 shadow-sm scrollbar-thin scrollbar-thumb-slate-300 dark:scrollbar-thumb-gray-600"
                            value={input}
                            onChange={(e) => {
                                setInput(e.target.value);
                                e.target.style.height = 'auto';
                                e.target.style.height = Math.min(e.target.scrollHeight, 120) + 'px';
                            }}
                            onKeyPress={handleKeyPress}
                            placeholder={t("Ask something...")}
                            rows={1}
                            disabled={isLoading}
                            style={{
                                minHeight: '48px',
                                maxHeight: '120px',
                                overflowY: input.split('\n').length > 3 ? 'auto' : 'hidden'
                            }}
                        />
                    </div>
                    <button
                        className={`flex-shrink-0 w-12 h-12 rounded-xl flex items-center justify-center transition-all duration-200 shadow-lg ${input.trim() && !isLoading
                            ? "bg-gradient-to-br from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white hover:shadow-xl transform hover:scale-105 active:scale-95"
                            : "bg-slate-200 dark:bg-gray-600 text-slate-400 dark:text-gray-400 cursor-not-allowed"
                            }`}
                        onClick={sendMessage}
                        disabled={!input.trim() || isLoading}
                    >
                        {isLoading ? (
                            <div className="w-5 h-5 border-2 border-white border-t-transparent rounded-full animate-spin"></div>
                        ) : (
                            <FiSend className="text-xl" />
                        )}
                    </button>
                </div>
            </div>
        </div>
    );
};

export default ChatBot;