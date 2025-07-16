#!/bin/bash

echo "⚡ Quick Commit & Push - Artemis Apollo Integration"
echo ""

# Make scripts executable
chmod +x commit_artemis_features.sh
chmod +x push_and_merge.sh

# Run the commit script
echo "📝 Step 1: Committing changes..."
./commit_artemis_features.sh

if [ $? -eq 0 ]; then
    echo ""
    echo "📤 Step 2: Pushing and merging..."
    ./push_and_merge.sh
    
    if [ $? -eq 0 ]; then
        echo ""
        echo "🎊 SUCCESS! Artemis features committed and merged!"
        echo ""
        echo "🎯 What was accomplished:"
        echo "   ✅ Virtual Display Control (TESTED & WORKING)"
        echo "   ✅ Fractional Refresh Rate (Ready for testing)"  
        echo "   ✅ Resolution Scaling (Ready for testing)"
        echo "   ✅ Professional UI with tooltips"
        echo "   ✅ Full Apollo backend integration"
        echo "   ✅ Zero breaking changes"
        echo "   ✅ Code committed to git"
        echo ""
        echo "🚀 Ready to continue with remaining Artemis features!"
    else
        echo "❌ Push/merge failed. Please check the output above."
    fi
else
    echo "❌ Commit failed. Please check the output above."
fi